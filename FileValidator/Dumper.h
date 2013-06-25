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
	
	std::wstring fileName_;
	ImportTable import_;
	IMAGE_NT_HEADERS *imageHeader_;
	LPVOID fileMapAddress_;
	Export exportFuncCache_;
		
	typedef std::pair<std::wstring, IMAGE_SECTION_HEADER*> Section;
	std::vector<Section> section_;	
		
	void Dump();
	void GetImportTable();
	void GetDelayImportTable();
	DWORD GetImageBase(std::wstring fileName);
	std::string GetDllFunctionNameByOrdinal(std::wstring LibName, WORD ordinal);	
	std::wstring GetMachineSpecific();
	Strings GetCharacteristics();
	std::wstring GetMagic();
	std::wstring GetSubsystem();
	Strings GetDllCharacteristic();
	SectionInfo GetSectionInfo(DWORD id);
	Strings GetSectionCharacteristics(DWORD id);
	DWORD RvaToFileOffset(const DWORD rva) const;
	DWORD RvaToFileOffset(const DWORD rva, const std::vector<Section> sections) const;
	void GetSectionHeaders(std::vector<Section>& sections);
	BOOL CheckImportFunction(std::wstring libName, const std::wstring funcName);
	void SetCurrentDirectory();
	void Dumper::GetLibraryExportDirectory(std::wstring libName, std::vector<std::string>& funcList);
public:	
	Dumper(std::wstring fileName): fileName_(fileName) 
	{
		section_.resize(15);
		section_[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].first = L"Architecture-specific data";
		section_[IMAGE_DIRECTORY_ENTRY_BASERELOC].first = L"Base relocation table";
		section_[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].first = L"Bound import directory";
		section_[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].first = L"COM descriptor table";		
		section_[IMAGE_DIRECTORY_ENTRY_DEBUG].first = L"Debug directory";
		section_[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].first = L"Delay import table";
		section_[IMAGE_DIRECTORY_ENTRY_EXCEPTION].first = L"Exception directory";
		section_[IMAGE_DIRECTORY_ENTRY_EXPORT].first = L"Export directory";
		section_[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].first = L"The relative virtual address of global pointer";
		section_[IMAGE_DIRECTORY_ENTRY_IAT].first = L"Import address table";
		section_[IMAGE_DIRECTORY_ENTRY_IMPORT].first = L"Import directory";
		section_[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].first = L"Load configuration directory";
		section_[IMAGE_DIRECTORY_ENTRY_RESOURCE].first = L"Resource directory";
		section_[IMAGE_DIRECTORY_ENTRY_SECURITY].first = L"Security directory";
		section_[IMAGE_DIRECTORY_ENTRY_TLS].first = L"Thread local storage directory";
		
		SetCurrentDirectory();
		Dump();
		GetImportTable();
		GetDelayImportTable();
	}
	~Dumper(void) 
	{
		UnmapViewOfFile(fileMapAddress_);
	}
		
	void ShowData(InfoTable table);
	void ShowSections(InfoTable table);
	void ShowImportTable(InfoTable table);
};