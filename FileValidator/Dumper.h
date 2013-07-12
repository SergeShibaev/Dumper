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
	typedef std::map<std::wstring, std::vector<std::string> > Export;	// dllName -> export functions list : 
	
	HANDLE hFileMap_;
	LPVOID fileMapAddress_;
	IMAGE_NT_HEADERS *imageHeader_;
	std::wstring fileName_;
	ImportTable import_;	
	mutable Export exportFuncCache_;
		
	typedef std::pair<std::wstring, IMAGE_SECTION_HEADER*> Section;
	std::vector<Section> section_;

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
	BOOL CheckImportFunction(std::wstring& libName, const std::string& funcName) const;
	void SetCurrentDirectory();
	void GetLibraryExportDirectory(const std::wstring& libName, std::vector<std::string>& funcList) const;
	// TODO: if import has function for create file or write registry values Dumper should find all filenames and keys/values
public:	
	Dumper(std::wstring fileName, BOOL logging): fileName_(fileName) 
	{
		section_.resize(16);
		section_[IMAGE_DIRECTORY_ENTRY_EXPORT].first = L"Export directory";
		section_[IMAGE_DIRECTORY_ENTRY_IMPORT].first = L"Import directory";
		section_[IMAGE_DIRECTORY_ENTRY_RESOURCE].first = L"Resource directory";
		section_[IMAGE_DIRECTORY_ENTRY_EXCEPTION].first = L"Exception directory";
		section_[IMAGE_DIRECTORY_ENTRY_SECURITY].first = L"Security directory";		
		section_[IMAGE_DIRECTORY_ENTRY_BASERELOC].first = L"Base relocation table";
		section_[IMAGE_DIRECTORY_ENTRY_DEBUG].first = L"Debug directory";
		section_[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].first = L"Architecture-specific data";
		section_[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].first = L"The relative virtual address of global pointer";
		section_[IMAGE_DIRECTORY_ENTRY_TLS].first = L"Thread local storage directory";
		section_[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].first = L"Load configuration directory";
		section_[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].first = L"Bound import directory";
		section_[IMAGE_DIRECTORY_ENTRY_IAT].first = L"Import address table";		
		section_[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].first = L"Delay import table";
		section_[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].first = L"COM descriptor table";
		section_[15].first = L"Reserved";

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
	}
	~Dumper(void) 
	{
		if (fileMapAddress_)
			UnmapViewOfFile(fileMapAddress_);
		if (hFileMap_ != INVALID_HANDLE_VALUE)
			CloseHandle(hFileMap_);
	}
		
	void ShowData(InfoTable table);
	void ShowSections(InfoTable table);
	void ShowImportTable(InfoTable table);
	void CheckDependencies() const;

	static void SplitPath(const LPWSTR fileName, LPWSTR path, LPWSTR file);
};