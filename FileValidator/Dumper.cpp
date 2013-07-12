#include "stdafx.h"
#include "Dumper.h"
#include "StrUtils.h"
#include <Psapi.h>
#include <stdexcept>

#pragma comment(lib, "DbgHelp.lib")

const std::string Dumper::UNKNOWN_FUNCTION = "N/A";
const std::wstring Dumper::wUNKNOWN_FUNCTION = L"N/A";

std::wstring Dumper::GetFullFileName(const std::wstring& fileName) const
{
	// TODO: find path to the side-by-side components like comctl32.dll and gdiplus.dll by reading version from registry
	std::wstring fullFileName;

	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		fullFileName = fileName;
		CloseHandle(hFile);
	}
	else 
	{
		fullFileName.resize(MAX_PATH);
		SearchPath(NULL, fileName.c_str(), NULL, MAX_PATH, &fullFileName[0], NULL);		
	}
	
	return fullFileName;
}

void Dumper::LoadFileAsImage()
{	
	fileName_ = GetFullFileName(fileName_);

	HANDLE hFile = CreateFile(fileName_.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		logger_.AddLog(L"ОШИБКА при загрузке файла: " + fileName_);
		return;
	}
	logger_.AddLog(L"Загружен файл: " + fileName_);

	hFileMap_ = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	CloseHandle(hFile);
	if (hFileMap_ == NULL)
	{
		logger_.AddLog(L"ОШИБКА при отображении файла в память: " + fileName_);
		return;
	}
	fileMapAddress_ = MapViewOfFile(hFileMap_, FILE_MAP_READ, 0, 0, 0);
}

void Dumper::ReadHeader()
{	
	imageHeader_ = ImageNtHeader(fileMapAddress_);
	if (imageHeader_ == NULL)
	{
		logger_.AddLog(L"ОШИБКА: Заголовок NT пустой");
		return;
	}
	if (imageHeader_->Signature != IMAGE_NT_SIGNATURE)
	{
		logger_.AddLog(L"ОШИБКА: Сигнатура NT отсутствует");
		return;
	}

	DWORD  size;
	for (USHORT i = 0; i < section_.size(); ++i)
		ImageDirectoryEntryToDataEx(fileMapAddress_, FALSE, i, &size, &section_[i].second);
}

std::string Dumper::GetDllFunctionNameByOrdinal(const std::wstring& libName, const WORD ordinal) const
{
	if (exportFuncCache_[libName].size() == 0)
		GetLibraryExportDirectory(libName, exportFuncCache_[libName]);

	if (exportFuncCache_[libName].size() > ordinal)
		return exportFuncCache_[libName][ordinal];
	else
		return UNKNOWN_FUNCTION;
}

void Dumper::GetLibraryExportDirectory(const std::wstring& libName, std::vector<std::string>& funcList) const
{
	funcList.clear();
	Dumper libDump(libName, FALSE);
	if (!libDump.imageHeader_)
	{
		logger_.AddLog(L"ОШИБКА: Не удалось загрузить библиотеку " + libDump.fileName_ + L" для чтения секции эскпорта");
		return;
	}
	
	DWORD size;
	LPVOID pData = ImageDirectoryEntryToDataEx(libDump.fileMapAddress_, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size, NULL);
	PIMAGE_EXPORT_DIRECTORY exportTable = (PIMAGE_EXPORT_DIRECTORY)pData;
	PDWORD names = (PDWORD)libDump.ImageRvaToVa(exportTable->AddressOfNames);
	PWORD ordinals = (PWORD)((PDWORD)libDump.ImageRvaToVa(exportTable->AddressOfNameOrdinals));		
	funcList.resize(exportTable->NumberOfFunctions + exportTable->Base);
	for (USHORT i = 0; i < exportTable->NumberOfNames; ++i)
	{			
		DWORD curNameRva = *names++;						
		std::string curName = (PCHAR)libDump.ImageRvaToVa(curNameRva);
		
		DWORD curOrd = exportTable->Base + *ordinals++;
		if (curOrd >= funcList.size())
			funcList.resize(curOrd+1);
		funcList[curOrd] = curName;		
	}
	PULONG functions = (PULONG)libDump.ImageRvaToVa(exportTable->AddressOfFunctions);
	DWORD exportSection = libDump.imageHeader_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD exportSize = libDump.imageHeader_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	for (USHORT i = 0; i < exportTable->NumberOfFunctions; ++i)
	{
		DWORD funcRva = (DWORD)libDump.ImageRvaToVa(*functions);
		if (*functions >= exportSection && *functions < exportSection + exportSize)
			funcList[exportTable->Base + i] = (PCHAR)funcRva;
		functions++;
	}
}

void Dumper::SplitPath(const LPWSTR fileName, LPWSTR path, LPWSTR file)
{
	WCHAR drive[5] = { 0 }, dir[MAX_PATH] = { 0 }, fname[MAX_PATH] = { 0 }, ext[MAX_PATH] = { 0 };
		
	_wsplitpath_s(fileName, drive, dir, fname, ext);
	if (path)
	{
		StringCchPrintf(path, MAX_PATH, L"%s%s", drive, dir);
		if (path[wcslen(path)-1] == '\\')
			path[wcslen(path)-1] = '\0';
	}

	if (file)
		StringCchPrintf(file, MAX_PATH, L"%s%s", fname, ext);
}

void Dumper::SetCurrentDirectory()
{
	WCHAR path[MAX_PATH];
	SplitPath((LPWSTR)fileName_.c_str(), path, NULL);
	::SetCurrentDirectory(path);
}

void Dumper::ReadImportFull()
{
	if (imageHeader_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress)
	{
		DWORD timeDateStamp = imageHeader_->FileHeader.TimeDateStamp;
		// bound import only for timeDateStamp equals the same in DLL
		ReadBoundImportTable();
	}
	else
	{
		GetImportTable();
	}
	/*
	ReadIATDirectory();*/
	GetDelayImportTable();
}

BOOL Dumper::CheckImportFunction(std::wstring& libName, const std::string& funcName) const
{
	// TODO: resolve why some functions like 'ntdll.RtlVirtualUnwind' has not been found in export table or via GetProcAddress
	if (exportFuncCache_[libName].size() == 0)
		GetLibraryExportDirectory(libName, exportFuncCache_[libName]);
		
	BOOL funcExists = funcName == UNKNOWN_FUNCTION ||
		std::find(exportFuncCache_[libName].begin(), exportFuncCache_[libName].end(), funcName) != exportFuncCache_[libName].end();

	if (!funcExists)
	{
		HMODULE hDll = LoadLibrary(libName.c_str());
		LPVOID func = GetProcAddress(hDll, funcName.c_str());
		funcExists = (func != NULL);
		FreeLibrary(hDll);
	}

	return funcExists;
}

template<typename T>
void Dumper::ReadImportedFunctions(const DWORD rva, const std::wstring& libName , std::vector<FUNCTION_INFO>& funcList) const
{
	T thunkRef = (T)ImageRvaToVa(rva);
	BOOL x32 = sizeof(thunkRef->u1) == sizeof(IMAGE_THUNK_DATA32);
	BOOL x64 = sizeof(thunkRef->u1) == sizeof(IMAGE_THUNK_DATA64);	
	while (thunkRef->u1.AddressOfData)
	{
		FUNCTION_INFO finfo = { "", EMPTY_ORDINAL, EMPTY_HINT };
		if (x32 && IMAGE_SNAP_BY_ORDINAL32(thunkRef->u1.Ordinal) ||
			x64 && IMAGE_SNAP_BY_ORDINAL64(thunkRef->u1.Ordinal))
		{
			std::string funcName = GetDllFunctionNameByOrdinal(libName, IMAGE_ORDINAL(thunkRef->u1.Ordinal));
			if (funcName == "")
				funcName = UNKNOWN_FUNCTION;
			finfo.name = funcName;
			finfo.ordinal = IMAGE_ORDINAL(thunkRef->u1.Ordinal);
		}
		else 
		{
			PIMAGE_IMPORT_BY_NAME data = (PIMAGE_IMPORT_BY_NAME)ImageRvaToVa(thunkRef->u1.AddressOfData & 0xFFFFFFFF);
			finfo.name = data->Name;
			finfo.hint = data->Hint;
		}
		funcList.push_back(finfo);
		thunkRef++;
	}
}

void Dumper::GetImportTable()
{
	logger_.AddLog(L"Чтение таблицы импорта процесса...");

	DWORD size;
	PIMAGE_IMPORT_DESCRIPTOR desc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(fileMapAddress_, FALSE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, NULL);
	while (desc && desc->Characteristics)
	{
		LibExport libExp;
		PCHAR libName = (PCHAR)ImageRvaToVa(desc->Name);
		libExp.first = Convert(libName);
						
		if (imageHeader_->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			ReadImportedFunctions<PIMAGE_THUNK_DATA32>(desc->OriginalFirstThunk, libExp.first, libExp.second);
		else if (imageHeader_->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			ReadImportedFunctions<PIMAGE_THUNK_DATA64>(desc->OriginalFirstThunk, libExp.first, libExp.second);
				
		import_.push_back(libExp);		
		desc++;

		WCHAR msg[MAX_PATH + 50] = { 0 };			
		StringCchPrintf(msg, MAX_PATH + 50, L"%ls -> импортировано функций: %d", 
			GetFullFileName(libExp.first).c_str(), libExp.second.size());
		logger_.AddLog(msg);
	}
}

void Dumper::ReadIATDirectory()
{
	IMAGE_DATA_DIRECTORY section = imageHeader_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
	if (!section.VirtualAddress)
		return;

	
	LibExport le;
	le.first = L"IAT.dll";
	import_.push_back(le);
}

void Dumper::ReadBoundImportTable()
{		
	IMAGE_DATA_DIRECTORY section = imageHeader_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];	
	if (!section.VirtualAddress)
		return;
	
	PIMAGE_BOUND_IMPORT_DESCRIPTOR desc = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)ImageRvaToVa(section.VirtualAddress);
	PCHAR libName = (PCHAR)ImageRvaToVa(section.VirtualAddress + desc->OffsetModuleName);
}

void Dumper::GetDelayImportTable()
{
	DWORD size;
	PIMAGE_DELAYLOAD_DESCRIPTOR desc = (PIMAGE_DELAYLOAD_DESCRIPTOR)ImageDirectoryEntryToDataEx(fileMapAddress_, FALSE, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, &size, NULL);
	
	while (desc && desc->Attributes.AllAttributes)
	{
		LibExport libExp;
		PCHAR libName = (PCHAR)ImageRvaToVa(desc->DllNameRVA);
		libExp.first = Convert(libName);

		if (imageHeader_->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			ReadImportedFunctions<PIMAGE_THUNK_DATA32>(desc->ImportNameTableRVA, libExp.first, libExp.second);
		else if (imageHeader_->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			ReadImportedFunctions<PIMAGE_THUNK_DATA64>(desc->ImportNameTableRVA, libExp.first, libExp.second);
		
		import_.push_back(libExp);
		desc++;
	}
}

void Dumper::CheckDependencies() const
{
	logger_.AddLog(L"\nПроверка зависимостей...");
	std::deque<std::wstring> libs;
	std::vector<std::wstring> checked;
	
	for (USHORT i = 0; i < import_.size(); ++i)
	{		
		std::wstring libName = import_[i].first;
		libs.push_back(libName);
		for (USHORT j = 0; j < import_[i].second.size(); ++j)
		{
			std::string funcName = import_[i].second[j].name;
			if (!CheckImportFunction(libName, funcName))
				logger_.AddLog(L"ОШИБКА: не удалось получить адрес функции: " + libName + L"." + Convert(funcName));
		}
	}

	while (libs.size())
	{
		std::wstring libName = libs[0];
		libs.pop_front();

		if (std::find(checked.begin(), checked.end(), libName) != checked.end())
			continue;

		Dumper lib(libName, FALSE);
		lib.ReadImportFull();
		if (lib.import_.size() != 0)
			logger_.AddLog(L"\n" + libName);
		
		for (USHORT i = 0; i < lib.import_.size(); ++i)
		{
			std::wstring impLibName = lib.import_[i].first;
			logger_.AddLog(L"|__" + GetFullFileName(impLibName));
			libs.push_back(impLibName);

			for (USHORT j = 0; j < lib.import_[i].second.size(); ++j)
			{
				std::string funcName = lib.import_[i].second[j].name;
				if (!CheckImportFunction(impLibName, funcName))
					logger_.AddLog(L"|____ ОШИБКА: не удалось получить адрес функции " + Convert(funcName));
			}
		}
		

		checked.push_back(libName);
		
	}
}

std::wstring Dumper::GetMachineSpecific() const
{
	switch (imageHeader_->FileHeader.Machine)
	{
	case IMAGE_FILE_MACHINE_I386: return L"x86";
	case IMAGE_FILE_MACHINE_IA64: return L"Intel Itanium";
	case IMAGE_FILE_MACHINE_AMD64: return L"x64";
	default: return L"Unknown machine";
	}
}

Dumper::Strings Dumper::GetCharacteristics() const
{
	Strings result;
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
		result.push_back(L"Relocation information was stripped from the file");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
		result.push_back(L"The file is executable");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_LINE_NUMS_STRIPPED)
		result.push_back(L"COFF line numbers were stripped from the file");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED)
		result.push_back(L"COFF symbol table entries were stripped from file");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM)
		result.push_back(L"Aggressively trim the working set");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
		result.push_back(L"The application can handle addresses larger than 2 GB");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_BYTES_REVERSED_LO)
		result.push_back(L"The bytes of the word are reversed");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_32BIT_MACHINE)
		result.push_back(L"The computer supports 32-bit words");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
		result.push_back(L"Debugging information was removed and stored separately in another file");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP)
		result.push_back(L"If the image is on removable media");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP)
		result.push_back(L"If the image is on the network");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_SYSTEM)
		result.push_back(L"The image is a system file");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_DLL)
		result.push_back(L"The image is a DLL file");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
		result.push_back(L"The file should be run only on a uniprocessor computer");
	if (imageHeader_->FileHeader.Characteristics & IMAGE_FILE_BYTES_REVERSED_HI)
		result.push_back(L"The bytes of the word are reversed");

	return result;
}

std::wstring Dumper::GetMagic() const
{
	switch (imageHeader_->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return L"The file is an executable image";
	case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
		return L"The file is a ROM image";
	default:
		return L"Unknown magic";
	}
}

std::wstring Dumper::GetSubsystem() const
{
	switch (imageHeader_->OptionalHeader.Subsystem)
	{
	case IMAGE_SUBSYSTEM_NATIVE: return L"No subsystem required";
	case IMAGE_SUBSYSTEM_WINDOWS_GUI: return L"Windows graphical user interface (GUI) subsystem";
	case IMAGE_SUBSYSTEM_WINDOWS_CUI: return L"Windows character-mode user interface (CUI) subsystem";
	case IMAGE_SUBSYSTEM_OS2_CUI: return L"OS/2 CUI subsystem";
	case IMAGE_SUBSYSTEM_POSIX_CUI: return L"POSIX CUI subsystem";
	case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI: return L"Windows CE system";
	case IMAGE_SUBSYSTEM_EFI_APPLICATION: return L"Extensible Firmware Interface (EFI) application";
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER: return L"EFI driver with boot services";
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER: return L"EFI driver with run-time services";
	case IMAGE_SUBSYSTEM_EFI_ROM: return L"EFI ROM image";
	case IMAGE_SUBSYSTEM_XBOX: return L"Xbox system";
	case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION: return L"Boot application";
	default: return L"Unknown subsystem";
	}
}

Dumper::Strings Dumper::GetDllCharacteristic() const
{
	Strings result;
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)
		result.push_back(L"The DLL can be relocated at load time");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)
		result.push_back(L"Code integrity checks are forced");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)
		result.push_back(L"The image is compatible with data execution prevention (DEP)");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_ISOLATION)
		result.push_back(L"The image is isolation aware, but should not be isolated");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_SEH)
		result.push_back(L"The image does not use structured exception handling (SEH)");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_BIND)
		result.push_back(L"Do not bind the image");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER)
		result.push_back(L"A WDM driver");
	if (imageHeader_->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
		result.push_back(L"The image is terminal server aware");

	return result;
}

Dumper::SectionInfo Dumper::GetSectionInfo(DWORD id) const
{
	IMAGE_SECTION_HEADER *section = section_[id].second;
	SectionInfo result;
	if (section != NULL)
	{
		result.push_back(std::make_pair(L"Name", Convert(reinterpret_cast<CHAR*>(section->Name))));
		result.push_back(std::make_pair(L"Misc.PhysicalAddress", ValueAsHex(section->Misc.PhysicalAddress)));
		result.push_back(std::make_pair(L"Misc.VirtualSize", ValueAsStr(section->Misc.VirtualSize)));
		result.push_back(std::make_pair(L"VirtualAddress", ValueAsHex(section->VirtualAddress)));
		result.push_back(std::make_pair(L"SizeOfRawData", ValueAsStr(section->SizeOfRawData)));
		result.push_back(std::make_pair(L"PointerToRawData", ValueAsHex(section->PointerToRawData)));
		result.push_back(std::make_pair(L"PointerToRelocations", ValueAsHex(section->PointerToRelocations)));
		result.push_back(std::make_pair(L"PointerToLinenumbers", ValueAsHex(section->PointerToLinenumbers)));
		result.push_back(std::make_pair(L"NumberOfRelocations", ValueAsStr(section->NumberOfRelocations)));
		result.push_back(std::make_pair(L"NumberOfLinenumbers", ValueAsStr(section->NumberOfLinenumbers)));
		result.push_back(std::make_pair(L"Characteristics", ValueAsHex(section->Characteristics)));
	}	

	return result;
}

Dumper::Strings Dumper::GetSectionCharacteristics(DWORD id) const
{
	IMAGE_SECTION_HEADER *section = section_[id].second;
	Strings result;
	if (section != NULL)
	{
		if (section->Characteristics & IMAGE_SCN_TYPE_NO_PAD)
			result.push_back(L"The section should not be padded to the next boundary");
		if (section->Characteristics & IMAGE_SCN_CNT_CODE)
			result.push_back(L"The section contains executable code");
		if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			result.push_back(L"The section contains initialized data");
		if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			result.push_back(L"The section contains uninitialized data");
		if (section->Characteristics & IMAGE_SCN_LNK_INFO)
			result.push_back(L"The section contains comments or other information");
		if (section->Characteristics & IMAGE_SCN_LNK_REMOVE)
			result.push_back(L"The section will not become part of the image");
		if (section->Characteristics & IMAGE_SCN_LNK_COMDAT)
			result.push_back(L"The section contains COMDAT data");
		if (section->Characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC)
			result.push_back(L"Reset speculative exceptions handling bits in the TLB entries");
		if (section->Characteristics & IMAGE_SCN_GPREL)
			result.push_back(L"The section contains data referenced through the global pointer");
		if (section->Characteristics & IMAGE_SCN_ALIGN_1BYTES)
			result.push_back(L"Align data on a 1-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_2BYTES)
			result.push_back(L"Align data on a 2-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_4BYTES)
			result.push_back(L"Align data on a 4-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_8BYTES)
			result.push_back(L"Align data on a 8-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_16BYTES)
			result.push_back(L"Align data on a 16-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_32BYTES)
			result.push_back(L"Align data on a 32-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_64BYTES)
			result.push_back(L"Align data on a 64-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_128BYTES)
			result.push_back(L"Align data on a 128-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_256BYTES)
			result.push_back(L"Align data on a 256-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_512BYTES)
			result.push_back(L"Align data on a 512-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_1024BYTES)
			result.push_back(L"Align data on a 1024-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_2048BYTES)
			result.push_back(L"Align data on a 2048-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_4096BYTES)
			result.push_back(L"Align data on a 4096-byte boundary");
		if (section->Characteristics & IMAGE_SCN_ALIGN_8192BYTES)
			result.push_back(L"Align data on a 8192-byte boundary");
		if (section->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
			result.push_back(L"The section contains extended relocations");
		if (section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
			result.push_back(L"The section can be discarded as needed");
		if (section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
			result.push_back(L"The section cannot be cached");
		if (section->Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
			result.push_back(L"The section cannot be paged");
		if (section->Characteristics & IMAGE_SCN_MEM_SHARED)
			result.push_back(L"The section can be shared in memory");
		if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
			result.push_back(L"The section can be executed as code");
		if (section->Characteristics & IMAGE_SCN_MEM_READ)
			result.push_back(L"The section can be read");
		if (section->Characteristics & IMAGE_SCN_MEM_WRITE)
			result.push_back(L"The section can be written to");
	}
	return result;
}

std::wstring GetJoinedVersion(DWORD major, DWORD minor)
{
	WCHAR version[20];
	StringCchPrintf(version, 20, L"%d.%d", major, minor);
	return version;
}

void Dumper::ShowData(InfoTable table)
{		
	table.DeleteAllItems();
	table.AppendItem(L"===== IMAGE_NT_HEADER");
	WCHAR signature[20] = { 0 };
	StringCchPrintf(signature, 20, L"%c%c   (0x%X)", 
		(char)(imageHeader_->Signature & 0xFF), (char)(imageHeader_->Signature >> 8), imageHeader_->Signature);
	table.AppendItem(L"Signature", signature);
	
	table.AppendItem(L"===== IMAGE_FILE_HEADER");
	IMAGE_FILE_HEADER ifh = imageHeader_->FileHeader;
	table.AppendItem(L"Machine", GetMachineSpecific());
	table.AppendItem(L"NumberOfSections", ifh.NumberOfSections);
	table.AppendItem(L"TimeDateStamp", ifh.TimeDateStamp);
	table.AppendItem(L"PointerToSymbolTable", ifh.PointerToSymbolTable);
	table.AppendItem(L"NumberOfSymbols", ifh.NumberOfSymbols);
	table.AppendItem(L"SizeOfOptionalHeader", ifh.SizeOfOptionalHeader);
	table.AppendItem(L"Characteristics", ifh.Characteristics);
	
	Strings characteristics = GetCharacteristics();
	for (USHORT i = 0; i < characteristics.size(); ++i)
		table.AppendItem(L"", characteristics[i]);

	IMAGE_OPTIONAL_HEADER ioh = imageHeader_->OptionalHeader;
	table.AppendItem(L"===== IMAGE_OPTIONAL_HEADER");
	table.AppendItem(L"Magic", ioh.Magic);
	table.AppendItem(L"", GetMagic());
	table.AppendItem(L"LinkerVersion", GetJoinedVersion(ioh.MajorLinkerVersion, ioh.MinorLinkerVersion));	
	table.AppendItem(L"SizeOfCode", ioh.SizeOfCode);
	table.AppendItem(L"SizeOfInitializedData", ioh.SizeOfInitializedData);
	table.AppendItem(L"SizeOfUninitializedData", ioh.SizeOfUninitializedData);
	table.AppendItem(L"AddressOfEntryPoint", ioh.AddressOfEntryPoint);
	table.AppendItem(L"BaseOfCode", ioh.BaseOfCode);
#if !defined(_WIN64)	
	table.AppendItem(L"BaseOfData", ioh.BaseOfData);
#endif
	table.AppendItem(L"ImageBase", ioh.ImageBase);
	table.AppendItem(L"SectionAlignment", ioh.SectionAlignment);
	table.AppendItem(L"FileAlignment", ioh.FileAlignment);
	table.AppendItem(L"OperatingSystemVersion", 
		GetJoinedVersion(ioh.MajorOperatingSystemVersion, ioh.MinorOperatingSystemVersion));
	table.AppendItem(L"ImageVersion", GetJoinedVersion(ioh.MajorImageVersion, ioh.MinorImageVersion));	
	table.AppendItem(L"SubsystemVersion", GetJoinedVersion(ioh.MajorSubsystemVersion, ioh.MinorSubsystemVersion));
	table.AppendItem(L"Win32VersionValue", ioh.Win32VersionValue);
	table.AppendItem(L"SizeOfImage", ioh.SizeOfImage);
	table.AppendItem(L"SizeOfHeaders", ioh.SizeOfHeaders);
	table.AppendItem(L"CheckSum", ioh.CheckSum);
	table.AppendItem(L"Subsystem", ioh.Subsystem);
	table.AppendItem(L"", GetSubsystem());
	table.AppendItem(L"DllCharacteristics", ioh.DllCharacteristics);
	
	characteristics = GetDllCharacteristic();
	for (USHORT i = 0; i < characteristics.size(); ++i)
		table.AppendItem(L"", characteristics[i]);

	table.AppendItem(L"SizeOfStackReserve", ioh.SizeOfStackReserve);
	table.AppendItem(L"SizeOfStackCommit", ioh.SizeOfStackCommit);
	table.AppendItem(L"SizeOfHeapReserve", ioh.SizeOfHeapReserve);
	table.AppendItem(L"SizeOfHeapCommit", ioh.SizeOfHeapCommit);
	table.AppendItem(L"LoaderFlags", ioh.LoaderFlags);
	table.AppendItem(L"NumberOfRvaAndSizes", ioh.NumberOfRvaAndSizes);	
}

void Dumper::ShowSections(InfoTable table)
{
	table.DeleteAllItems();
	for (USHORT i = 0; i < section_.size(); ++i)
	{
		IMAGE_SECTION_HEADER *section = section_[i].second;
		if (section == NULL)
			continue;
				
		DWORD line = table.GetItemCount();
		table.AppendItem(ValueAsHex(i));
		table.InsertSubitem(line, 1, Convert(reinterpret_cast<CHAR*>(section->Name)));
		table.InsertSubitem(line, 2, ValueAsHex(section->Misc.VirtualSize));
		table.InsertSubitem(line, 3, ValueAsHex(section->VirtualAddress));		
		table.InsertSubitem(line, 4, ValueAsHex(section->PointerToRawData));
		table.InsertSubitem(line, 5, section_[i].first);
	}
}

void Dumper::ShowImportTable(InfoTable table)
{
	ReadImportFull();
	
	table.DeleteAllItems();
	for (USHORT i = 0; i < import_.size(); ++i)
	{
		if (i > 0) 
			table.AppendItem(L"");

		for (USHORT j = 0; j < import_[i].second.size(); ++j)
		{
			FUNCTION_INFO func = import_[i].second[j];

			DWORD line = table.GetItemCount();
			table.AppendItem(import_[i].first);
			table.InsertSubitem(line, 1, Convert(func.name));
			if (func.ordinal != EMPTY_ORDINAL)
				table.InsertSubitem(line, 2, ValueAsHex(func.ordinal));
			else
				table.InsertSubitem(line, 2, L"");
			if (func.hint != EMPTY_HINT)
				table.InsertSubitem(line, 3, ValueAsHex(func.hint));
			else
				table.InsertSubitem(line, 3, L"");
			if (CheckImportFunction(import_[i].first, func.name))
				table.InsertSubitem(line, 4, L"+");
			else
				table.InsertSubitem(line, 4, L"-");
		}
	}
}