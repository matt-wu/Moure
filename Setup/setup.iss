[Setup]
AppName=Moure
AppVerName=Moure 1.3
AppPublisher=Matt Wu
AppCopyright=Copyright (c) 2016 Matt Wu
AppPublisherURL=http://www.ext2fsd.com
AppSupportURL=http://www.ext2fsd.com
AppUpdatesURL=http://www.ext2fsd.com
DefaultDirName={pf}\Moure
DefaultGroupName=Moure
AllowNoIcons=true
OutputDir=D:\Projects\Moure\bin
OutputBaseFilename=Moure-1.3
Compression=lzma
SolidCompression=true
InternalCompressLevel=max
SourceDir=D:\Projects\Moure
VersionInfoVersion=1.38.08.06
VersionInfoCompany=Ext2Fsd Group
AllowUNCPath=false
DisableReadyPage=true
ShowLanguageDialog=auto
MinVersion=0,5.1.2600
AppVersion=1.3
VersionInfoDescription=Moure V1.3 Setup
UninstallRestartComputer=true
Uninstallable=true
ArchitecturesInstallIn64BitMode=x64


[Files]
Source: Manager\objfre_wxp_x86\i386\Moure.exe; DestDir: {app}; Flags: ignoreversion; Components: driver
Source: Filter\winxp\fre\i386\moure.sys; DestDir: {sys}\Drivers; Components: driver; Check: not Is64BitInstallMode; MinVersion: 0,5.01.2600; Flags: restartreplace ignoreversion uninsrestartdelete promptifolder
Source: Filter\winnet\fre\amd64\moure.sys; DestDir: {sys}\Drivers; Components: driver; Check: Is64BitInstallMode; MinVersion: 0,5.01.2600; Flags: restartreplace ignoreversion uninsrestartdelete promptifolder

[Icons]
Name: {group}\Swap Your Mouse Button; Filename: {app}\moure.exe; WorkingDir: {app}
Name: {group}\Uninstall Moure; Filename: "{uninstallexe}"; WorkingDir: {app}

[Run]
Filename: {app}\moure.exe; Description: Configure your mouses right now ?; Flags: runascurrentuser nowait postinstall skipifsilent; WorkingDir: {app}

[UninstallRun]

[Types]
Name: custom; Description: Moure Core; Flags: iscustom

[Components]
Name: driver; Description: Moure Core; ExtraDiskSpaceRequired: 1177; Flags: fixed; Types: custom

[Tasks]

[Registry]
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Services\moure; ValueType: string;  ValueName: DisplayName; ValueData: Moure Filter for Mouse HID; Flags: uninsdeletekey
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Services\moure; ValueType: dword;   ValueName: ErrorControl; ValueData: 00000001; Flags: uninsdeletekey
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Services\moure; ValueType: dword;   ValueName: Start; ValueData: 00000001; Flags: uninsdeletekey;
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Services\moure; ValueType: dword;   ValueName: Type; ValueData: 00000001; Flags: uninsdeletekey
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Services\moure; ValueType: multisz; ValueName: MouseHWs; ValueData: ; Flags: deletevalue uninsdeletekey

[Code]

type
	SERVICE_STATUS = record
    	dwServiceType				: cardinal;
    	dwCurrentState				: cardinal;
    	dwControlsAccepted			: cardinal;
    	dwWin32ExitCode				: cardinal;
    	dwServiceSpecificExitCode	: cardinal;
    	dwCheckPoint				: cardinal;
    	dwWaitHint					: cardinal;
	end;

	ENUM_SERVICE_STATUS = record
		strServiceName:PAnsiChar;
		strDisplayName:PAnsiChar;
		Status:			SERVICE_STATUS;
	end;
	HANDLE = cardinal;
const
	ENUM_SERVICE_STATUS_SIZE   = 36;

const SERVICE_ERROR_NORMAL		=1;
const
	SERVICE_QUERY_CONFIG		= $1;
	SERVICE_CHANGE_CONFIG		= $2;
	SERVICE_QUERY_STATUS		= $4;
	SERVICE_START				= $10;
	SERVICE_STOP				= $20;
	SERVICE_ALL_ACCESS			= $f01ff;
	SC_MANAGER_ALL_ACCESS		= $f003f;
	SERVICE_KERNEL_DRIVER		=$1;
	SERVICE_FILE_SYSTEM_DRIVER	= $2;
	SERVICE_WIN32_OWN_PROCESS	= $10;
	SERVICE_WIN32_SHARE_PROCESS	= $20;
	SERVICE_WIN32				= $30;
	SERVICE_INTERACTIVE_PROCESS = $100;
	SERVICE_BOOT_START          = $0;
	SERVICE_SYSTEM_START        = $1;
	SERVICE_AUTO_START          = $2;
	SERVICE_DEMAND_START        = $3;
	SERVICE_DISABLED            = $4;
	SERVICE_DELETE              = $10000;
	SERVICE_CONTROL_STOP		= $1;
	SERVICE_CONTROL_PAUSE		= $2;
	SERVICE_CONTROL_CONTINUE	= $3;
	SERVICE_CONTROL_INTERROGATE = $4;
	SERVICE_STOPPED				= $1;
	SERVICE_START_PENDING       = $2;
	SERVICE_STOP_PENDING        = $3;
	SERVICE_RUNNING             = $4;
	SERVICE_CONTINUE_PENDING    = $5;
	SERVICE_PAUSE_PENDING       = $6;
	SERVICE_PAUSED              = $7;
	SERVICE_NO_CHANGE            =  $ffffffff;

function ControlService(hService :HANDLE; dwControl :cardinal;var ServiceStatus :SERVICE_STATUS) : boolean;
external 'ControlService@advapi32.dll stdcall';

function CloseServiceHandle(hSCObject :HANDLE): boolean;
external 'CloseServiceHandle@advapi32.dll stdcall';

function DeleteService(hService :HANDLE): boolean;
external 'DeleteService@advapi32.dll stdcall';

function CreateService(hSCManager :HANDLE;lpServiceName, lpDisplayName: string;dwDesiredAccess,dwServiceType,dwStartType,dwErrorControl: cardinal;lpBinaryPathName,lpLoadOrderGroup: String; lpdwTagId : cardinal;lpDependencies,lpServiceStartName,lpPassword :string): cardinal;
external 'CreateServiceA@advapi32.dll stdcall';

function OpenService(hSCManager :HANDLE;lpServiceName: string; dwDesiredAccess :cardinal): HANDLE;
external 'OpenServiceA@advapi32.dll stdcall';

function OpenSCManager(lpMachineName, lpDatabaseName: string; dwDesiredAccess :cardinal): HANDLE;
external 'OpenSCManagerA@advapi32.dll stdcall';

function QueryServiceStatus(hService :HANDLE;var ServiceStatus :SERVICE_STATUS) : boolean;
external 'QueryServiceStatus@advapi32.dll stdcall';

function ChangeServiceConfig(hService:HANDLE;dwServiceType,dwStartType,dwErrorControl:cardinal;lpBinaryPathName,lpLoadOrderGroup: String;lpdwTagId : cardinal;lpDependencies,lpServiceStartName,lpPassword ,lpDisName:string):boolean;
external 'ChangeServiceConfigA@advapi32.dll stdcall';

function StartService(hService:HANDLE;nNumOfpara:DWORD;strParam:String):boolean;
external 'StartServiceA@advapi32.dll stdcall';

function EnumDependentServices(hService:HANDLE;dwServiceState:DWORD;var Service_Status : Array of ENUM_SERVICE_STATUS; cbBufSize:DWORD; var pcbByteNeeded, lpServicesReturned:DWORD):boolean;
external 'EnumDependentServicesA@advapi32.dll stdcall';

function GetLastError():DWORD;
external 'GetLastError@Kernel32.dll stdcall';



function OpenServiceManager() : HANDLE;
begin
	if UsingWinNT() = true then begin
		Result := OpenSCManager('','ServicesActive',SC_MANAGER_ALL_ACCESS);
		if Result = 0 then
			MsgBox('the servicemanager is not available', mbError, MB_OK)
	end
	else begin
			MsgBox('only nt based systems support services', mbError, MB_OK)
			Result := 0;
	end
end;

function IsServiceInstalled(ServiceName: string) : boolean;
var
	hSCM	: HANDLE;
	hService: HANDLE;
begin
	hSCM := OpenServiceManager();
	Result := false;
	if hSCM <> 0 then begin
		hService := OpenService(hSCM,ServiceName,SERVICE_QUERY_CONFIG);
        if hService <> 0 then begin
            Result := true;
            CloseServiceHandle(hService)
		end;
        CloseServiceHandle(hSCM)
	end
end;

function IsServiceRunning(ServiceName: string) : boolean;
var
	hSCM	: HANDLE;
	hService: HANDLE;
	Status	: SERVICE_STATUS;
begin
	hSCM := OpenServiceManager();
	Result := false;
	if hSCM <> 0 then begin
		hService := OpenService(hSCM,ServiceName,SERVICE_QUERY_STATUS);
    	if hService <> 0 then begin
			if QueryServiceStatus(hService,Status) then begin
				Result :=(Status.dwCurrentState = SERVICE_RUNNING)
        	end;
            CloseServiceHandle(hService)
		    end;
        CloseServiceHandle(hSCM)
	end
end;

function StopService(ServiceName: string) : boolean;
var
	hSCM	: HANDLE;
	hService: HANDLE;
	Status	: SERVICE_STATUS;
	i 		: integer;
begin
	hSCM := OpenServiceManager();
	Result := false;

	//OutputString('=============Service Name=====:'+ServiceName+' =============================='+#10+#13);
	if hSCM <> 0 then begin
		hService := OpenService(hSCM,ServiceName,SERVICE_STOP);
        if hService <> 0 then begin
        	Result := ControlService(hService,SERVICE_CONTROL_STOP,Status);
            CloseServiceHandle(hService)
		end;
        CloseServiceHandle(hSCM)

        // ÑÓ¾¡¼ì²é3´Î
        for i:=1 to 3 do
        begin
         if( IsServiceRunning(ServiceName)) then
         begin
			Sleep(3000);
		 end
		 else
		 begin
		  exit;
		 end;
        end;


	end;
end;

function RunService(ServiceName: string) : boolean;
var
	hSCM	: HANDLE;
	hService: HANDLE;
	Status	: SERVICE_STATUS;

begin
	hSCM := OpenServiceManager();
	Result := false;
	if hSCM <> 0 then begin
		hService := OpenService(hSCM,ServiceName,SERVICE_START);
        if hService <> 0 then begin
        	Result := StartService(hService,0,'');
            CloseServiceHandle(hService)
		end;
        CloseServiceHandle(hSCM)


	end;
end;


function RemoveService(ServiceName: string) : boolean;
var
	hSCM	: HANDLE;
	hService: HANDLE;
	Status	: SERVICE_STATUS;

begin
	hSCM := OpenServiceManager();
	Result := false;
	if hSCM <> 0 then begin
		hService := OpenService(hSCM,ServiceName,SERVICE_DELETE);
        if hService <> 0 then begin
        	Result := DeleteService(hService);
            RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE,'SYSTEM\CurrentControlSet\Services\'+ServiceName);
            sleep(100);
            RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE,'SYSTEM\CurrentControlSet\Services\'+ServiceName);
		end;
        CloseServiceHandle(hSCM)
	end;
end;

function InstallService(FileName, ServiceName, DisplayName, Description : string;ServiceType,StartType :cardinal;szDepends:string) : boolean;
var
	hSCM	: HANDLE;
	hService: HANDLE;
begin
	hSCM := OpenServiceManager();
	Result := false;
	if hSCM <> 0 then begin
		hService := CreateService(hSCM,ServiceName,DisplayName,SERVICE_ALL_ACCESS,ServiceType,StartType,SERVICE_ERROR_NORMAL,FileName,'',0,szDepends,'','');
		if hService <> 0 then begin
			Result := true;
			CloseServiceHandle(hService)
		end;
    CloseServiceHandle(hSCM)
	end
end;

function NeedRestart(): Boolean;
var
  strValue:string;

begin
    InstallService(expandconstant('{sys}\Drivers\moure.sys'), 'moure', 'Moure Filter for Moust HID', 'Moure Filter for Moust HID', SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START,#0);
    sleep(100);
    RegDeleteValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Services\moure', 'WOW64');
    // add moure into mouclass upperfilters
    RegQueryMultiStringValue(HKEY_LOCAL_MACHINE,'SYSTEM\CurrentControlSet\Control\Class\{4d36e96f-e325-11ce-bfc1-08002be10318}', 'UpperFilters',strValue);
    if (Pos('moure',Lowercase(strValue)) = 0) then
    begin
      strValue:=trim(strValue);
      if( Length(strValue)>0) then
      begin
        strValue:= strValue+#0+'moure'+#0;
      end
      else
      begin
        strValue:='moure'+#0;
      end;
      RegWriteMultiStringValue(HKEY_LOCAL_MACHINE,'SYSTEM\CurrentControlSet\Control\Class\{4d36e96f-e325-11ce-bfc1-08002be10318}', 'UpperFilters',strValue);
    end;

    RunService('moure');
    result := true;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin

if (CurStep = ssInstall) then
begin
  Exec('taskkill.exe', '/f /im moure.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  sleep(1000);
end;

if (CurStep = ssPostInstall) then 
begin
end;

end;


procedure CurUninstallStepChanged (CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
  strValue:string;
begin

if (CurUninstallStep = usUninstall) then 
begin

  Exec('taskkill.exe', '/f /im moure.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;


if (CurUninstallStep = usPostUninstall) then
begin

  //delete kdbpusr on wpd class
  RegQueryMultiStringValue(HKEY_LOCAL_MACHINE,'SYSTEM\CurrentControlSet\Control\Class\{4d36e96f-e325-11ce-bfc1-08002be10318}', 'UpperFilters',strValue);
  if (Pos('moure',Lowercase(strValue)) > 0) then
  begin
    StringChangeEx(strValue,'moure'+#0,'',true);
    strValue:=trim(strValue);
    strValue:=strValue+#0;
    RegWriteMultiStringValue(HKEY_LOCAL_MACHINE,'SYSTEM\CurrentControlSet\Control\Class\{4d36e96f-e325-11ce-bfc1-08002be10318}', 'UpperFilters',strValue);
  end;

  RemoveService('moure');
  Sleep(1000);
  DeleteFile(ExpandConstant('{sys}\Drivers\moure.sys'));
  Sleep(1000);
  DelTree(ExpandConstant('{app}'), True, True, True);
end;

end;
