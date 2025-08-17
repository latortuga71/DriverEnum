
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma comment(lib,"ntdll")

// Object attributes
#define 	NT_SUCCESS(Status)   (((NTSTATUS)(Status)) >= 0)
#define STATUS_MORE_ENTRIES 0x00000105
#define OBJ_INHERIT 0x00000002
#define OBJ_PERMANENT 0x00000010
#define OBJ_EXCLUSIVE 0x00000020
#define OBJ_CASE_INSENSITIVE 0x00000040
#define OBJ_OPENIF 0x00000080
#define OBJ_OPENLINK 0x00000100
#define OBJ_KERNEL_HANDLE 0x00000200
#define OBJ_FORCE_ACCESS_CHECK 0x00000400
#define OBJ_VALID_ATTRIBUTES 0x000007f2
typedef struct _STRING64
{
     USHORT Length;
     USHORT MaximumLength;
     ULONGLONG Buffer;
 } STRING64, * PSTRING64;

typedef STRING64 UNICODE_STRING64, * PUNICODE_STRING64;
typedef STRING64 ANSI_STRING64, * PANSI_STRING64;

typedef struct _OBJECT_ATTRIBUTES
{
     ULONG Length;
     HANDLE RootDirectory;
     PUNICODE_STRING64 ObjectName;
     ULONG Attributes;
     PVOID SecurityDescriptor; // PSECURITY_DESCRIPTOR;
     PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
 } OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES* PCOBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) { (p)->Length = sizeof(OBJECT_ATTRIBUTES);(p)->RootDirectory = r; (p)->Attributes = a; (p)->ObjectName = n; (p)->SecurityDescriptor = s; (p)->SecurityQualityOfService = NULL;}

#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a) { sizeof(OBJECT_ATTRIBUTES), NULL, n, a, NULL, NULL }
#define RTL_INIT_OBJECT_ATTRIBUTES(n, a) RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)
char buffer[1024];


VOID RtlInitUnicodeString( PUNICODE_STRING64 DestinationString, PWSTR SourceString) {
	if (SourceString) {
		DestinationString->MaximumLength = (DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR))) + sizeof(WCHAR);
	}
	else {
		DestinationString->MaximumLength = DestinationString->Length = 0;
	}
	DestinationString->Buffer = (ULONGLONG)SourceString;
}

extern "C" NTSTATUS NtOpenDirectoryObject(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
extern "C" NTSTATUS NtQueryDirectoryObject(HANDLE, PVOID, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG);
extern "C" NTSTATUS NtClose(HANDLE);
extern "C" NTSTATUS NtOpenSymbolicLinkObject(HANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
extern "C" NTSTATUS NtQuerySymbolicLinkObject(HANDLE, PUNICODE_STRING64, PULONG);

int Error(const char* msg) {
    printf("%s (%u)\n", msg, GetLastError());
    return -1;
}
#define DIRECTORY_QUERY 0x1
#define DIRECTORY_TRAVERSE 0x2

typedef struct _OBJECT_DIRECTORY_INFORMATION {
	UNICODE_STRING64 Name;
	UNICODE_STRING64 TypeName;
}OBJECT_DIRECTORY_INFORMATION,*POBJECT_DIRECTORY_INFORMATION;


BOOLEAN CheckPerm(WCHAR* name) {
	memset(buffer, 0, sizeof(buffer));
	std::wstring string_template = L"\\\\?\\GLOBALROOT\\Device\\";
	std::wstring path = string_template + name;
	HANDLE hDevice = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	else {
		std::wcout << L"[+] Access " << path << std::endl;
		CloseHandle(hDevice);
		return TRUE;
	}
}

std::wstring GetSymbolicLinkTarget(WCHAR* symname) {
	HANDLE handle;
	UNICODE_STRING64 name;
	RtlInitUnicodeString(&name, symname);
	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &name, 0, nullptr, nullptr);
	NTSTATUS status = NtOpenSymbolicLinkObject(&handle, GENERIC_READ, &objAttr);
	if (!NT_SUCCESS(status)) {
		return L"";
	}
	WCHAR buffer[512];
	UNICODE_STRING64 target;
	RtlInitUnicodeString(&target, buffer);
	target.MaximumLength = sizeof(buffer);
	status = NtQuerySymbolicLinkObject(handle, &target, nullptr);
	NtClose(handle);
	return buffer;
}

void EnumerateDrivers() {
	HANDLE handle;
	UNICODE_STRING64 name;
	RtlInitUnicodeString(&name, (PWSTR)L"\\Device");
	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &name, 0, nullptr, nullptr);
	NTSTATUS status = NtOpenDirectoryObject(&handle, DIRECTORY_QUERY, &objAttr);
	if (status != 0) {
		std::cout << "failed to opened dir!" << std::endl;
		return;
	}
	UINT8 queryBuffer[1024 * 4]; // 4kb
	POBJECT_DIRECTORY_INFORMATION data = (POBJECT_DIRECTORY_INFORMATION)queryBuffer;
	int start = 0;
	BOOLEAN first = true;
	ULONG index = 0;
	for (;;) {
		status = NtQueryDirectoryObject(handle, queryBuffer, sizeof(queryBuffer), FALSE, first, &index, nullptr);
		if (!NT_SUCCESS(status)){
			break;
		}
		first = false;
		for (ULONG i = 0; i < index - start; i++) {
			std::wstring type_name = (WCHAR*)data[i].TypeName.Buffer;
			std::wstring name = (WCHAR*)data[i].Name.Buffer;
			if (type_name == L"SymbolicLink") {
				std::wstring target = GetSymbolicLinkTarget((PWSTR)name.c_str());
				printf("Symbolic Link %ws -> %ws\n", (WCHAR*)data[i].Name.Buffer, name.c_str());
			}
			if (data[i].Name.Length > sizeof(WCHAR)) {
				//printf("DEBUG: %ws\n", (wchar_t*)data[i].Name.Buffer);
				CheckPerm((WCHAR*)data[i].Name.Buffer);
			}
		}
		start = index;
		if (status != STATUS_MORE_ENTRIES) {
			break;
		}
	}

	return;
}


VOID EnumFromFile(const char* path) {
	memset(buffer, 0, sizeof(buffer));
	std::ifstream ifs(path);
	std::string line;
	std::string string_template = "\\\\?\\GLOBALROOT\\Device\\";
	while (std::getline(ifs, line)) {
		std::string path = string_template + line;
		HANDLE hDevice = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hDevice == INVALID_HANDLE_VALUE) {
			//return Error("Failed to open driver");
			continue;
		}
		else {
			std::cout << "[+] Access " << path << std::endl;
			CloseHandle(hDevice);
		}
	}
}

int main(int argc, char* argv[]) {
	//EnumerateDrivers();
	EnumFromFile("c:\\users\\lator\\Desktop\\driverlist.txt");
}
