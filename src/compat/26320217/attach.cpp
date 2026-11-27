#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include "current_build.h"
struct Module {std::wstring name,path; std::uintptr_t base; DWORD size;};
std::vector<Module> Modules(DWORD pid){std::vector<Module> v;HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);if(s==INVALID_HANDLE_VALUE){std::cerr<<"Module snapshot failed: "<<GetLastError()<<'\n';return v;}MODULEENTRY32W m{sizeof(m)};if(Module32FirstW(s,&m))do{v.push_back({m.szModule,m.szExePath,reinterpret_cast<std::uintptr_t>(m.modBaseAddr),m.modBaseSize});}while(Module32NextW(s,&m));CloseHandle(s);return v;}
std::uintptr_t ModuleBase(DWORD pid,const std::wstring& name){for(auto&m:Modules(pid))if(!_wcsicmp(m.name.c_str(),name.c_str()))return m.base;return 0;}
bool RemoteCall(HANDLE process,std::uintptr_t fn,void* arg,DWORD& code){HANDLE t=CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(fn),arg,0,nullptr);if(!t){std::cerr<<"CreateRemoteThread failed: "<<GetLastError()<<'\n';return false;}DWORD wait=WaitForSingleObject(t,30000);bool ok=wait==WAIT_OBJECT_0&&GetExitCodeThread(t,&code);CloseHandle(t);if(!ok)std::cerr<<"Remote operation did not finish. Do not retry while it may still be running.\n";return ok;}
bool LoadRemote(HANDLE process,DWORD pid,const std::filesystem::path& path){
 if(ModuleBase(pid,path.filename().wstring()))return true;
 HMODULE owner{};auto load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(load),&owner))return false;
 wchar_t ownerPath[MAX_PATH]{};GetModuleFileNameW(owner,ownerPath,MAX_PATH);auto remoteOwner=ModuleBase(pid,std::filesystem::path(ownerPath).filename().wstring());if(!remoteOwner)return false;
 auto remoteLoad=remoteOwner+reinterpret_cast<std::uintptr_t>(load)-reinterpret_cast<std::uintptr_t>(owner);
 auto text=path.wstring();auto bytes=(text.size()+1)*sizeof(wchar_t);void* buffer=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!buffer)return false;SIZE_T written=0;
 if(!WriteProcessMemory(process,buffer,text.c_str(),bytes,&written)||written!=bytes){VirtualFreeEx(process,buffer,0,MEM_RELEASE);return false;}
 DWORD code=0;bool done=RemoteCall(process,remoteLoad,buffer,code);if(done)VirtualFreeEx(process,buffer,0,MEM_RELEASE);
 return done&&ModuleBase(pid,path.filename().wstring());
}
int wmain(int argc,wchar_t**argv){
 if(argc<2){std::cout<<"Usage: SledgePortAttach.exe PROCESS_ID [--stereo | --disable]\nDefault: install pass-through renderer probe; game files are not modified.\n";return 2;}
 DWORD pid=0;try{pid=std::stoul(argv[1]);}catch(...){std::cerr<<"Invalid process ID\n";return 2;}
 std::wstring mode=argc>2?argv[2]:L"";if(mode!=L""&&mode!=L"--stereo"&&mode!=L"--disable"){std::cerr<<"Unknown mode\n";return 2;}
 auto modules=Modules(pid);if(modules.empty()||_wcsicmp(modules.front().name.c_str(),L"teardown.exe")){std::cerr<<"Target is not teardown.exe\n";return 3;}
 if(ModuleBase(pid,L"sledge_core.dll")){std::cerr<<"Original Sledge is already loaded. Start the game normally through Steam.\n";return 3;}
 HANDLE process=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);if(!process){std::cerr<<"OpenProcess failed: "<<GetLastError()<<'\n';return 4;}
 struct Close{HANDLE h;~Close(){CloseHandle(h);}} close{process};
 if(modules.front().size!=CurrentBuild::ImageSize){std::cerr<<"Wrong game build; no DLL loaded.\n";return 5;}
 wchar_t own[MAX_PATH]{};GetModuleFileNameW(nullptr,own,MAX_PATH);auto dir=std::filesystem::path(own).parent_path();auto dll=dir/L"SledgePort21.dll";
 bool bridgeLoaded=false;
 for(auto&m:modules)if(!_wcsicmp(m.name.c_str(),L"SledgePort21.dll")){
  if(_wcsicmp(m.path.c_str(),dll.c_str())){std::cerr<<"A different renderer bridge is loaded; restart the game.\n";return 5;}
  bridgeLoaded=true;
 }
 for(auto&s:CurrentBuild::Sites){
  // Detours replaces the renderer entry after our verified bridge loads.
  if(bridgeLoaded&&CurrentBuild::IsHookSite(s.rva))continue;
  std::array<std::uint8_t,16> b{};SIZE_T n=0;if(!ReadProcessMemory(process,reinterpret_cast<void*>(modules.front().base+s.rva),b.data(),b.size(),&n)||n!=b.size()||b!=s.bytes){std::cerr<<"Build signature mismatch: "<<s.name<<". No DLL loaded.\n";return 5;}
 }
 if(!std::filesystem::exists(dll)||!std::filesystem::exists(dir/L"openvr_api.dll")){std::cerr<<"Required DLL missing beside the tool\n";return 6;}
 if(!LoadRemote(process,pid,dir/L"openvr_api.dll")||!LoadRemote(process,pid,dll)){std::cerr<<"DLL load could not be verified. Windows error "<<GetLastError()<<'\n';return 7;}
 std::cout<<"Renderer bridge loaded. See SledgePort21.log; this is an experimental build.\n";
 if(!mode.empty()){
  HMODULE local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);if(!local)return 8;
  const char* name=mode==L"--stereo"?"SledgePortEnableStereo":"SledgePortDisableStereo";auto entry=GetProcAddress(local,name);if(!entry){FreeLibrary(local);return 8;}
  auto rva=reinterpret_cast<std::uintptr_t>(entry)-reinterpret_cast<std::uintptr_t>(local);FreeLibrary(local);DWORD status=0;
  if(!RemoteCall(process,ModuleBase(pid,L"SledgePort21.dll")+rva,nullptr,status))return 9;
  if(status){std::cerr<<"Bridge not ready, status "<<status<<"; inspect the log before enabling stereo.\n";return 10;}
  std::cout<<(mode==L"--stereo"?"Stereo requested. Requires an OpenGL context and SteamVR headset.\n":"Stereo disabled; game rendering passes through unchanged.\n");
 }return 0;
}

