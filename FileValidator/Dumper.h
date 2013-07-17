#pragma once
#include "InfoTable.h"
#include "Logger.h"
#include <DbgHelp.h>

class Dumper
{
private:
	static const std::string UNKNOWN_FUNCTION;
	static const std::wstring wUNKNOWN_FUNCTION;
	static const DWORD EMPTY_ORDINAL = 0xFFFFFFFF;
	static const DWORD EMPTY_HINT = 0xFFFFFFFF;

	typedef struct {
		std::string name;
		DWORD ordinal;
		DWORD hint;
	} FUNCTION_INFO;

	typedef std::vector<std::wstring> Strings;
	typedef std::vector<std::pair<std::wstring, std::wstring> > SectionInfo;
	typedef std::pair<std::wstring, std::vector<FUNCTION_INFO> > LibExport;
	typedef std::vector<LibExport> ImportTable;
	typedef std::map<std::wstring, std::vector<std::string> > FuncList;	// dllName -> export functions list : 
		
	HANDLE hFileMap_;
	LPVOID fileMapAddress_;
	PIMAGE_NT_HEADERS imageHeader_;
	std::wstring fileName_;
	ImportTable import_;	
	mutable FuncList exportFuncCache_;
			
	std::vector<PIMAGE_SECTION_HEADER> section_;
	Strings dataDirDesc_;
	FuncList libraries_;

	mutable Logger logger_;
		
	void ReadHeader();
	std::wstring GetFullFileName(const std::wstring& fileName) const;
	void ReadImportFull();	
	void GetImportTable();
	void GetDelayImportTable();
	void ReadBoundImportTable();
	void ReadIATDirectory();
	template<typename T> void ReadImportedFunctions(const DWORD rva, const std::wstring& libName, std::vector<FUNCTION_INFO>& funcList) const;	
	//void ListSystemKnownDlls() const;     // move to the SystemInfo class
	
	LPVOID ImageRvaToVa(const ULONG rva) const { return ::ImageRvaToVa(imageHeader_, fileMapAddress_, rva, NULL); }
	void LoadFileAsImage();
	std::string GetDllFunctionNameByOrdinal(const std::wstring& LibName, const WORD ordinal) const;
	std::wstring GetMachineSpecific() const;
	Strings GetCharacteristics() const;
	std::wstring GetMagic() const;
	std::wstring GetSubsystem() const;
	Strings GetDllCharacteristic() const;
	SectionInfo GetSectionInfo(DWORD id) const;
	Strings GetSectionCharacteristics(DWORD id) const;	
	BOOL CheckImportFunction(const std::wstring& libName, const std::string& funcName) const;
	void SetCurrentDirectory();
	void GetLibraryExportDirectory(const std::wstring& libName, std::vector<std::string>& funcList) const;
	// TODO: if import has function for create file or write registry values Dumper should find all filenames and keys/values
public:	
	Dumper(std::wstring fileName, BOOL logging, BOOL wImport): fileName_(fileName) 
	{
		section_.resize(IMAGE_NUMBEROF_DIRECTORY_ENTRIES);
		dataDirDesc_.resize(IMAGE_NUMBEROF_DIRECTORY_ENTRIES);

		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_EXPORT] = L"Export directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_IMPORT] = L"Import directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_RESOURCE] = L"Resource directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_EXCEPTION] = L"Exception directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_SECURITY] = L"Security directory";		
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_BASERELOC] = L"Base relocation table";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_DEBUG] = L"Debug directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE] = L"Architecture-specific data";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_GLOBALPTR] = L"The relative virtual address of global pointer";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_TLS] = L"Thread local storage directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] = L"Load configuration directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] = L"Bound import directory";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_IAT] = L"Import address table";		
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] = L"Delay import table";
		dataDirDesc_[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] = L"COM descriptor table";
		dataDirDesc_[15] = L"Reserved";

		fileMapAddress_ = NULL;
		hFileMap_ = INVALID_HANDLE_VALUE;

		if (logging)
		{
			WCHAR file[MAX_PATH];
			SplitPath(&fileName_[0], NULL, file);
			std::wstring logFileName = L"dumper_" + std::wstring(file) + L".log";
			logger_.SetLogFile(logFileName);
		}
				
		LoadFileAsImage();
		ReadHeader();

		if (wImport)
			ReadImportFull();
	}
	~Dumper(void) 
	{
		if (fileMapAddress_)
			UnmapViewOfFile(fileMapAddress_);
		if (hFileMap_ != INVALID_HANDLE_VALUE)
			CloseHandle(hFileMap_);
		logger_.Save();
	}
		
	void ShowHeader(InfoTable& table) const;
	void ShowSections(InfoTable& table) const;
	void ShowDataDirs(InfoTable& table) const;
	void ShowImportTable(InfoTable& table) const;
	void ShowLibraries(InfoTable& table) const;
	void CheckDependencies();

	static void SplitPath(const LPWSTR fileName, LPWSTR path, LPWSTR file);
};