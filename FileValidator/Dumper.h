#pragma once
#include "InfoTable.h"
#include <DbgHelp.h>

class Dumper
{
private:
	static const std::string UNKNOWN_FUNCTION;
	static const std::wstring wUNKNOWN_FUNCTION;

	typedef std::vector<std::wstring> Strings;
	typedef std::vector<std::pair<std::wstring, std::wstring> > SectionInfo;
	typedef std::pair<std::wstring, Strings> LibExport;
	typedef std::vector<LibExport> ImportTable;
	typedef std::map<std::wstring, std::vector<std::string> > Export;	// dllName -> export functions list : 
	
	HANDLE hFileMap_;
	LPVOID fileMapAddress_;
	IMAGE_NT_HEADERS *imageHeader_;
	std::wstring fileName_;
	ImportTable import_;	
	Export exportFuncCache_;
		
	typedef std::pair<std::wstring, IMAGE_SECTION_HEADER*> Section;
	std::vector<Section> section_;	
		
	void Dump();
	void ReadImportFull();
	void GetImportTable();
	void GetDelayImportTable();
	void ReadBoundImportTable();
	void ReadIATDirectory();
	LPVOID ImageRvaToVa(const DWORD rva) { return ::ImageRvaToVa(imageHeader_, fileMapAddress_, rva, NULL); }
	DWORD GetImageBase();
	std::string GetDllFunctionNameByOrdinal(const std::wstring& LibName, const WORD ordinal);	
	std::wstring GetMachineSpecific() const;
	Strings GetCharacteristics() const;
	std::wstring GetMagic() const;
	std::wstring GetSubsystem() const;
	Strings GetDllCharacteristic() const;
	SectionInfo GetSectionInfo(DWORD id) const;
	Strings GetSectionCharacteristics(DWORD id) const;	
	BOOL CheckImportFunction(std::wstring& libName, const std::wstring& funcName);
	void SetCurrentDirectory();
	void GetLibraryExportDirectory(const std::wstring& libName, std::vector<std::string>& funcList);
public:	
	Dumper(std::wstring fileName): fileName_(fileName) 
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
		
		SetCurrentDirectory();
		Dump();		
	}
	~Dumper(void) 
	{
		UnmapViewOfFile(fileMapAddress_);
		CloseHandle(hFileMap_);
	}
		
	void ShowData(InfoTable table);
	void ShowSections(InfoTable table);
	void ShowImportTable(InfoTable table);
};