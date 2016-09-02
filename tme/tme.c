/* Executable name	: tme.exe
 * Version			: v1.0
 * Created Date		: 14.04.2016
 * Last Update		: 15.04.2016
 * Author			: Vladimir Dinev
 * Description		: A countdown timer program. It exists because I always forget my coffe on the stove.
 * The time is given in minutes as decimal numbers (e.g. 2.3 = 2 minutes and 18 seconds). 
 * A timer is set with the line
 * tme <time in minutes> "Your message"
 * The message needs to be enclosed in quotes if it contains more than one word.
 * By default the message is displayed in a message box and repeated vi the Windows speech syntesizer until
 * the message box is closed. /m after "Your message" will disable the voice, and only show you the message box.
 * /list /l -list and -l can be used to list active timers.
 * /stop /s -stop -s can be used to stop an active timer identified by it's number as listed by the list option. 
 * In the function descriptions all calls to the standard C library are omitted for brevity.
 * 
 * Built with		: TDM GCC (tdm64-1) version 5.1.0
 * IDE used			: Geany 1.26
 * Build line		: gcc -Wall -o "%e.exe" "%f" -s -m32 -lole32 -lWbemuuid -lOleAut32 -municode */

#include "tme.h"

//-------------------------------------------------------------------------
// main starts here
//-------------------------------------------------------------------------
int _tmain(int argc, _TCHAR * argv[])
{
/* Name			: _tmain()
 * Updated		: 15.04.2016
 * In			: The cmd line arguments.
 * Returns		: An int to the OS.
 * Modifies		: Nothing.
 * Calls		: Check_Arguments(), Set_Timer(), Start_Timer(), List_Timers(), Stop_Timer(), Print_Help(), Print_Options()
 * Description	: The main function is used to direct program flow depending on the cmd line arguments. */
 
	bool msgonly = false; 
	bool printable_msg = false;
	int timer_number, i;
	DWORD ttk_pid = 0;
	
	switch (Check_Arguments(argc, argv))
	{
		case SET_MSG_ONLY:
		
			/* If the message only option is set toggle msgonly
			 * and fall through to the SET procedure. */
			msgonly = true;
			
		case SET:
		
			// Go through message and see if contains any printable characters.
			for (i = 0; '\0' != argv[2][i]; ++i)
				if (_istgraph(argv[2][i])) {
						printable_msg = true;
						break;
				}
			
			// If it doesn't show error and exit.
			if (!printable_msg) {
				_putts(_T("Not a valid message."));
				return -1;
			}

			// Set the timer in a new background process.
			Set_Timer(argc, argv, msgonly);
			break;
			
		case START:
			
			/* 4th character of the 3rd argument will be either the 'm' 
			 * in /setm, or the '\0' in /set. */
			if ('m' == argv[3][4])
				Start_Timer(_tstof(argv[1]), argv[2], false);
			else
				Start_Timer(_tstof(argv[1]), argv[2], true);
			break;
			
		case LIST:
			
			// List active timers if availabe.
			List_Timers(argv, false, 0, NULL);
			break;
			
		case STOP:
			
			// Stop a timer.
			if ( (timer_number = _ttoi(argv[2])) <= 0 )
				_tprintf(_T("Second argument has to be a valid timer number.\n"));
			else
			{
				List_Timers(argv, true, timer_number, &ttk_pid);
				Stop_Timer(ttk_pid, timer_number);
			}	
			break;
			
		case ARG_ERR:
		
			_tprintf(_T("Invalid arguments.\n"));
			break;
			
		case HELP:
			
			Print_Help(argv);
			break;
			
		case OPTIONS:
			
			Print_Options(argv);
			break;
			
		default:
			break;
	}

	return 0;
}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------

char Check_Arguments(int argc, _TCHAR * argv[])
{
/* Name			: Check_Arguments()
 * Updated		: 15.04.2016
 * In			: The cmd line arguments.
 * Returns		: A numerical value stored in a char.
 * Modifies		: Nothing.
 * Calls		: Nothing.
 * Description	: Examines the cmd line arguments and returns a control code to _tmain(). */
 
	// Print help if no additional arguments are present.
	if (1 == argc)
		return HELP;
	
	/* If we have 2 additional arguments, and the first one is a digit
	 * set the timer. */
	if 	( (3 == argc) &&
		(('0' <= argv[1][0]) && ('9' >= argv[1][0])) ) 
		return SET;
	
	/* If we have 3 additional arguments and a time, check for the
	 * no message flag. */
	if ( (4 == argc) && 
		(('0' <= argv[1][0]) && ('9' >= argv[1][0])) && 
		(0 == _tcscmp(ARG_MSG_ONLY, argv[3])) )
		return SET_MSG_ONLY;
		
	/* If we have 3 additional arguments, and the last one is a /set
	 * option, start the timer. */	
	if ( (4 == argc) && 
		((0 == _tcscmp(ARG_SET, argv[3])) || 
		(0 == _tcscmp(ARG_SET_MSG_ONLY, argv[3]))) )
		return START;
	
	/* If the first character of the first additional argument is a
	 * '/' or a '-', check what's the argument. */	
	if ( (SLASH == argv[1][0]) || (DASH == argv[1][0]) )
	{	
		if ( 0 == _tcscmp(ARG_OPTIONS, argv[1]) )
			return OPTIONS;
		
		int i;
		for (i = 0; i < LIST_ARGS_COUNT; ++i) 
		{
			if ( 0 == _tcscmp(argv[1], ARGS_LIST[i]) )
				return LIST;
				
			if ( 0 == _tcscmp(argv[1], ARGS_STOP[i]) )
				return STOP;
		}
	}
	
	// If none of the above, return argument error code.
	return ARG_ERR;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void Set_Timer(int argc, _TCHAR * argv[], bool msgonly)
{
/* Name			: Set_Timer()
 * Updated		: 15.04.2016
 * In			: The cmd line arguments, and a boolean value indicating if the message will be spoken or not.
 * Returns		: Nothing.
 * Modifies		: Nothing.
 * Calls		: Create_Timer()
 * Description	: Creates the command line for the new process(which will work as the actual countdown timer)
 * 				  and prints a message for the user. */
	
	int i, arglen, minutes;
	float time, seconds;
	
	time = _tstof(argv[1]);
	// Check if first argument is a valid time.
	if (time <= 0 || time > (float)MAX_MINUTES)
	{
		_tprintf(TIME_ERROR_MSG, (int)MAX_MINUTES);
		return;
	}
	
	// Create the command line string for the new process:
	
	// Find out the lengths of all arguments.
	for (i = 0, arglen = 0; i < argc; ++i) 
		arglen += _tcslen(argv[i]);
		
	/* We add the number of arguments + 2 + 4 in order to make sure
	 * that we have space for the spaces(' '), quotes('"'), the /set or /setm option, 
	 * and the '\0' at the end, when we form the command line for the new process. */
	arglen += argc + 2 + 4;
	
	TCHAR cmdline[arglen];
	
	// Concatenate the new command line string.
	_tcscpy(cmdline, argv[0]);	// Executable name
	_tcscat(cmdline, _T(" "));
	_tcscat(cmdline, argv[1]);	// Time in minutes
	_tcscat(cmdline, _T(" "));
	_tcscat(cmdline, _T("\""));
	_tcscat(cmdline, argv[2]);	// Message
	_tcscat(cmdline, _T("\""));
	_tcscat(cmdline, _T(" "));
	
	// Add appropriate /set flag.
	if (msgonly) 
		_tcscat(cmdline, ARG_SET_MSG_ONLY);
	else
		_tcscat(cmdline, ARG_SET);
 
	// Create the new process.
	if ( Create_Timer(cmdline) )
	{
		// Calculate time values.
		time *= 60;
		minutes = (int)time / 60;
		seconds = (minutes > 0) ? (time - minutes * 60) : time;
		
		// Print "timer is set for..." message.
		_tprintf(_T("Timer is set for %d %s and %.*f %s from now.\n"),
		minutes,
		_T("min"),
		(seconds - (float)((int)seconds) >= 0.05) ? 1 : 0,
		seconds,
		_T("sec"));
		_tprintf(_T("Timer message is \"%s\".\n"), argv[2]);
	}
	else // Print error if Create_Timer() failed.
		_tprintf(_T("Timer could not be started.\n"));
	
	return;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool Create_Timer(TCHAR cmdline[])
{
/* Name			: Create_Timer()
 * Updated		: 15.04.2016
 * In			: The command line for the new process.
 * Returns		: Success of failure.
 * Modifies		: Nothing.
 * Calls		: WIN API ZeroMemory(), CreateProcess(), CloseHandle()
 * Description	: Creates a new process, passing it the command line composed in Set_Timer()
 * 				  The new process will be the actual countdown timer and will work indepedently in the background. */
 
	STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    if( !CreateProcess( NULL,   // No module name (use command line)
        cmdline,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | DETACHED_PROCESS, // Creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
		_tprintf(_T("New process could not be created.\n"));
        return false;
    }

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void Start_Timer(float time, TCHAR msg[], bool include_voice)
{	
/* Name			: Start_Timer()
 * Updated		: 15.04.2016
 * In			: A float value of the time in minutes, a pointer to the message string, and a boolean value
 * 				  indicating if the message will be spoken or not.
 * Returns		: Nothing.
 * Modifies		: Nothing.
 * Calls		: WIN API Sleep(), CoInitialize(), MessageBox(), CoCreateInstance(), CreateThread(), CloseHandle(), CoUninitialize()
 * Description	: Checks if the time value is valid, converts it to milliseconds and sleeps for that long. After that it checks
 * 				  if the timer message should be spoken, and if not it only displays a message box. If it is supposed to be
 * 				  spoken it initializes a Windows speech synthesizer COM object, creates a separate thread for the message box(since
 * 				  MessageBox() blocks the thread it runs on), and repeats the message until the message box is closed. */
	
	/* Another time check in case someone passes 
	 * /set or /setm manually. */
	if (time <= 0 || time > (float)MAX_MINUTES)
	{
		_tprintf(TIME_ERROR_MSG, (int)MAX_MINUTES);
		return;
	}
	
	// time * 60 * 1000 gives us the sleep time in seconds.
	Sleep(time * 60 * 1000);
	
	if (!include_voice)
	{
		// Display only a message box if the /m option is used.
		MessageBox(NULL, msg, TITLE, MB_ICONINFORMATION | MB_SYSTEMMODAL);
		return;
	}
	
	HRESULT hr = 0;
	
	// Initialize a Windows voice synthesizer COM object.
	hr = CoInitialize(NULL);
	if (FAILED(hr))
    {
		MessageBox(NULL, _T("Speech COM initialization failed.\n"), TITLE, MB_ICONERROR | MB_SYSTEMMODAL);
		CoUninitialize();
		return;
	}
	
	struct ISpVoice* pVoice = NULL;
	hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_ALL, &IID_ISpVoice, (void**)&pVoice);
	
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Speech COM CoCreateInstance failed.\n"), TITLE, MB_ICONERROR | MB_SYSTEMMODAL);
		CoUninitialize();
		return;
	}
	
	/* Covert msg to wide string.
	 * Add for ANSI only
	int requiredSize = mbstowcs(NULL, msg, 0);
	WCHAR wmsg[requiredSize + 1];
	mbstowcs(wmsg, msg, requiredSize + 1); */
		
	// Create thread struct.
	THREAD_MSG thrdmsg = { msg, true };
	HANDLE thread;
	
	// Run the message box thread.
	thread = CreateThread(NULL, 0, Show_Message, &thrdmsg, 0, NULL);
	
	// Loop while the message box is on the screen.
	while (thrdmsg.thread_running)
	{
		pVoice->lpVtbl->Speak(pVoice, msg, 0, NULL);
		Sleep(SPEAK_PAUSE);
	}	
	CloseHandle(thread);
	pVoice->lpVtbl->Release(pVoice);
	
    /* MSDN documentation says that even if CoInitalize fails, 
     * CoUnitialize must be called. */
    CoUninitialize();
	
	return; 
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
DWORD WINAPI Show_Message(LPVOID lpParam) 
{
/* Name			: Show_Message()
 * Updated		: 15.04.2016
 * In			: A void pointer later converted to a pointer to a THREAD_MSG struct.
 * Returns		: 32 bit unsigned int.
 * Modifies		: The thread_running member of the THREAD_MSG struct.
 * Calls		: WIN API MessageBox()
 * Description	: Shows the timer message in a message box and notifies the voice loop in Start_Timer()
 * 				  when the message box is closed. */
	
	THREAD_MSG *tmsg = (THREAD_MSG*)lpParam;

	// Show the message box.
	MessageBox(NULL, tmsg->msg, TITLE, MB_ICONINFORMATION | MB_SYSTEMMODAL);

	/* After MessageBox returns, toggle thread_running
	 * in order to exit the loop in Start_Timer(). */
	tmsg->thread_running = false;

	return 0;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool List_Timers(_TCHAR * argv[], bool kill, char timer_to_kill, DWORD * ttk_pid)
{
/* Name			: List_Timers()
 * Updated		: 15.04.2016
 * In			: The cmd line arguments, a boolean value indicating if the functions will be used to stop a timer,
 * 				  a char holding the ID of the timer to be stopped, and a pointer to it's PID.
 * Returns		: Success or failure.
 * Modifies		: The value at &ttk_pid if the kill option is set.
 * Calls		: Get_Image_Name(), Print_Timer_Info()
 * 				  WIN API GetCurrentProcessId(), CoInitializeEx(), CoInitializeSecurity(), CoCreateInstance(), CoUninitialize()
 * Description	: Enumerates the current running timers, and either calls Print_Timer_Info() or puts the PID of the timer
 * 				  to be killed at &ttk_pid. */

	HRESULT hr = 0;
    IWbemLocator         *WbemLocator  = NULL;
    IWbemServices        *WbemServices = NULL;
    IEnumWbemClassObject *EnumWbem  = NULL;
	TCHAR image_name[MAX_PATH];
	DWORD current_pid;
	bool are_there_timers = false;
	
	// Get image name.
	if( !Get_Image_Name(image_name) )
	{
		_tprintf(_T("Error: Could not get image name.\n"));
		return false;
	}
	
	current_pid = GetCurrentProcessId();
	
    // Initializate the Windows security.
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    
    if (FAILED(hr))
    {
		_tprintf(_T("CoInitializeEx failed.\n"));
		CoUninitialize();
		return false;
	}
		
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hr))
	{
		_tprintf(_T("CoInitializeSecurity failed.\n"));
		CoUninitialize();
		return false;
	}
	
	hr = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &WbemLocator);
	if (FAILED(hr))
	{
		_tprintf(_T("CoCreateInstance failed.\n"));
		CoUninitialize();
		return false;
	}
	
	// Connect to the WMI.
	hr = WbemLocator->lpVtbl->ConnectServer(WbemLocator, _T("ROOT\\CIMV2"), NULL, NULL, NULL, 0, NULL, NULL, &WbemServices);   
	if (FAILED(hr))
	{
		_tprintf(_T("Could not connect to server.\n"));
		WbemLocator->lpVtbl->Release(WbemLocator);
		CoUninitialize();
		return false;
		
	}	
	
	// Run the WQL Query.
	hr = WbemServices->lpVtbl->ExecQuery(WbemServices, _T("WQL"), _T("SELECT ProcessId, Name, CommandLine FROM Win32_Process"), 
	WBEM_FLAG_FORWARD_ONLY, NULL, &EnumWbem);
	if (FAILED(hr))
	{
		_tprintf(_T("Could not execute query.\n"));
		WbemServices->lpVtbl->Release(WbemServices);
		WbemLocator->lpVtbl->Release(WbemLocator);
		CoUninitialize();
		return false;
	}
	
	// Iterate over the enumerator.
	if (EnumWbem != NULL) 
	{
		IWbemClassObject *result = NULL;
		ULONG returnedCount = 0;
		int timer_number = 0;
		
		while((hr = EnumWbem->lpVtbl->Next(EnumWbem, WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK) 
		{
			VARIANT ProcessId;
			VARIANT CommandLine;
			VARIANT Name;

			// Access the properties.
			hr = result->lpVtbl->Get(result, _T("ProcessId"), 0, &ProcessId, 0, 0);
			hr = result->lpVtbl->Get(result, _T("CommandLine"), 0, &CommandLine, 0, 0);            
			hr = result->lpVtbl->Get(result, _T("Name"), 0, &Name, 0, 0);
			
			if ( !(CommandLine.vt == VT_NULL) && (0 == _tcscmp(Name.bstrVal, image_name)) && 
			(ProcessId.uintVal != current_pid) )
			{
				++timer_number;
				if (false == kill)
				{
					Print_Timer_Info(CommandLine.bstrVal, timer_number, ProcessId.uintVal);
					are_there_timers = true;
				}
				else
				{
					are_there_timers = true;
					if (timer_number == timer_to_kill)
						*ttk_pid = ProcessId.uintVal;	
				}
			}
			
			result->lpVtbl->Release(result);
		}
	}
	
	if (!are_there_timers)
		_tprintf(_T("No timers available.\n"));

	// Release the resources.
	EnumWbem->lpVtbl->Release(EnumWbem);
	WbemServices->lpVtbl->Release(WbemServices);
	WbemLocator->lpVtbl->Release(WbemLocator);

    CoUninitialize();
    
    return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void Print_Timer_Info(BSTR timer_cmd_line, int timer_number, DWORD pid)
{
/* Name			: Print_Timer_Info()
 * Updated		: 15.04.2016
 * In			: The cmd line of the enumerated from List_Timers() timer, it's number, and it's pid.
 * Returns		: Nothing.
 * Modifies		: Nothing.
 * Calls		: Calculate_Time_Left()
 * Description	: Examines the command line arguments of the enumerated from List_Timers() timer 
 * 				  and prints a frame buffer containing timer information. */

	int i, j, k, l, set_minutes, set_seconds, left_minutes, left_seconds;
	float time_in_min, calc_time;
	bool msg_speak = true;
	TCHAR szSetTimeInMinutes[10] = {_T(' ')}; 	// Significant time value can be at most 9 characters.
	TCHAR szMessage[256] = {_T(' ')}; 			// Cmd message.
	TCHAR szFrameBuff[512] = {_T(' ')}; 		// Frame buffer.
	

	// Get the start of the seconds argument, which is the time.
	for (i = 0; ' ' != timer_cmd_line[i]; ++i)
		continue;
	
	// Put it in the time buffer.
	for (++i, j = 0; ' ' != timer_cmd_line[i]; ++i, ++j)
		szSetTimeInMinutes[j] = timer_cmd_line[i];
	
	time_in_min = _tstof(szSetTimeInMinutes);
	
	// Print time in format mm:ss as well.
	set_minutes = (int)time_in_min;
	set_seconds = (int)(time_in_min * 60) % 60;

	// Get the start index of the message.
	for (i = 0; '"' != timer_cmd_line[i]; ++i)
		continue;
	
	// Put the message in the message buffer.
	++i; k = 0; l = sizeof(szMessage) - 2;
	do
	{
		szMessage[k] = timer_cmd_line[i];
		++k; ++i;
	}
	while ('"' != timer_cmd_line[i] && k < l);
	
	// Put a null at the end to terminate the string.
	szMessage[k] = _T('\0');
	
	// Get time left.
	calc_time = Calculate_Time_Left(pid, time_in_min);

	if (-1 == calc_time)
		_tprintf(_T("\nError in calculating the time left."));
	
	// Get left time in format mm:ss as well.
	left_minutes = ((int)calc_time * 60) / 60;
	left_seconds = (int)(calc_time * 60) % 60;
	
	// See if the message has been set with the message only flag.
	if ('m' == timer_cmd_line[_tcslen(timer_cmd_line) - 1])
		msg_speak = false;
	
	// Fill the frame buffer.
	_sntprintf(szFrameBuff, sizeof(szFrameBuff),
	_T("\n%d.\n%-11s %.2f min or %02d:%02d min\n%-11s %.2f min or %02d:%02d min\n%-11s %s \n%-11s %s\n"),
	timer_number, _T("Set for:"), time_in_min, set_minutes, set_seconds, _T("Time left:"), calc_time, 
	left_minutes, left_seconds, _T("Message:"), szMessage, _T("Voice:"), (msg_speak) ? _T("yes") : _T("no"));
	
	// Print it.
	_tprintf(_T("%s"), szFrameBuff);
	
	return;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
float Calculate_Time_Left(DWORD pid, float time_in_min)
{ 
/* Name			: Calculate_Time_Left()
 * Updated		: 15.04.2016
 * In			: The PID of a timer and it's set time in minutes.
 * Returns		: A float value of the remaining time in minutes.
 * Modifies		: Nothing.
 * Calls		: WIN API GetSystemTime(), OpenProcess(), GetProcessTimes(), SystemTimeToFileTime(), CloseHandle()
 * Description	: Calculates the time left by substracting the process creation time from the current system time
 * 				  and subsctracting the result from that from the amount of time the timer has been originally set. */
	
	FILETIME ftSys, ftCreationTime, ftExitTime, ftKernelTime, ftUserTime;
	ULARGE_INTEGER uliSys, uliProc;
	SYSTEMTIME st;
	HANDLE p;
	BOOL chk;
	
	float time_in_seconds;
	
	GetSystemTime(&st);
	p = OpenProcess(PROCESS_QUERY_INFORMATION, false, pid);
	if (!p)
	{
		_tprintf(_T("Could not open process.\n"));
		return -1;
	}
	
	chk = GetProcessTimes(p, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime);
	if (!chk)
	{
		_tprintf(_T("Could not get process time.\n"));
		return -1;
	}
	
	// Convert system time to file time so we can perform arithmetic.
	chk = SystemTimeToFileTime(&st, &ftSys);
	if (!chk)
	{
		_tprintf(_T("Could not convert system time.\n"));
		return -1;
	}
	
	// Place both file times in a ULARGE_INTEGER as per Microsoft documentation.
	uliSys.LowPart = ftSys.dwLowDateTime;
	uliSys.HighPart = ftSys.dwHighDateTime;
	
	uliProc.LowPart = ftCreationTime.dwLowDateTime;
	uliProc.HighPart = ftCreationTime.dwHighDateTime;
	
	/* Get the difference in seconds. Filetime holds the amount of times 100 nanoseconds have passed 
	 * since January 1, 1601 (UTC), hence the divison by 10 000 000 */
	time_in_seconds = (uliSys.QuadPart - uliProc.QuadPart) / 10000000;
	
	CloseHandle(p);
	
	// Return the time left.
	return time_in_min - (time_in_seconds / 60);
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool Get_Image_Name(TCHAR * image_name)
{
/* Name			: Get_Image_Name()
 * Updated		: 15.04.2016
 * In			: A pointer to the address where the image's name will be stored.
 * Returns		: Success or failure.
 * Modifies		: The value at &image_name.
 * Calls		: WIN API GetModuleFileName()
 * Description	: Gets the full path and image name, extracts only the image name, and writes it at &image_name. */
 
	TCHAR szImgFullName[MAX_PATH];
	
	// Get full module path and name.	
	if( !GetModuleFileName(NULL, szImgFullName, MAX_PATH) )
	{
		_tprintf(_T("GetModuleFileName failed.\n"));
		return false;
	}
	
	// Get length of the string.	
	int path_size, img_name_start;
	path_size = _tcslen(szImgFullName);
	
	/* szImgFullName[2] would be the delimiting character in the path
	 * string (e.g. the '\' in "C:\..\.."), so here we parse the 
	 * szImgFullName starting from the end and look for the first 
	 * delimiter. */
	for (img_name_start = path_size; szImgFullName[img_name_start] != szImgFullName[2]; 
	--img_name_start)
	continue; 
	
	/* After ++img_name_start we have the index of the first letter of 
	 * the image name. */
	++img_name_start;
	int i;
	for (i = 0; img_name_start <= path_size; ++i, ++img_name_start)
		image_name[i] = szImgFullName[img_name_start];
	
	return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void Stop_Timer(DWORD pid, char tmr_num)
{
/* Name			: Stop_Timer()
 * Updated		: 15.04.2016
 * In			: A PID and the timer's number.
 * Returns		: Nothing.
 * Modifies		: Nothing.
 * Calls		: WIN API OpenProcess(), TerminateProcess(), CloseHandle()
 * Description	: Kills the process identified by the PID. */
	
	if (0 == pid)
	{
		_tprintf(_T("There is no timer %d.\n"), tmr_num);
		return;
	}
	
	HANDLE p;
	BOOL chk;
	
	p = OpenProcess(PROCESS_TERMINATE, false, pid);
	if (!p)
	{
		_tprintf(_T("Could not open process.\n"));
		return;
	}

	chk = TerminateProcess(p, 0);
	if (!chk)
	{
		_tprintf(_T("Could not terminate process.\n"));
		return;
	}
	
	_tprintf(_T("Timer %d has been stopped.\n"), tmr_num);
	
	CloseHandle(p);
	
	return;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void Print_Help(_TCHAR * argv[])
{
/* Name			: Print_Help()
 * Updated		: 15.04.2016
 * In			: The cmd line arguments.
 * Returns		: Nothing.
 * Modifies		: Nothing.
 * Calls		: Nothing.
 * Description	: Prints help info when the program is called with no arguments. */
	
	
	_tprintf(_T("%-15s %s\n"), _T("Name:"), NAME);
	_tprintf(_T("%-14s %s\n"), _T("Version:"), VERSION);
	_tprintf(_T("%-15s Sets a countdown timer and notifies you when the time is up.\n"), _T("Description:"));
	_tprintf(_T("%-15s %s <time in minutes> <your message> \n"), _T("Use:"), argv[0]);
	_tprintf(_T("%-15s %s 5 \"Your coffee is ready.\"\n"), _T("Example:"), argv[0]);
	_tprintf(_T("%-15s %s %s\n"), _T("For options:"), argv[0], ARG_OPTIONS); 
	_tprintf(_T("%-15s For times less than a minute use a decimal number\n%-15s e.g. 0.5 for 30 seconds.\n"), _T("Remarks:"), _T(" "));
	_tprintf(_T("%-15s Messages are spoken by default.\n"), _T(" "));
	
	return;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void Print_Options(_TCHAR * argv[])
{
/* Name			: Print_Options()
 * Updated		: 15.04.2016
 * In			: The cmd line arguments.
 * Returns		: Nothing.
 * Modifies		: Nothing.
 * Calls		: Nothing.
 * Description	: Prints the available options when the program is called with the /? argument. */
	
	const TCHAR example[] = _T("Example:");
	
	_tprintf(_T("To list running timers use %s, %s, %s, or %s\n"), ARGS_LIST[0], ARGS_LIST[1], ARGS_LIST[2], ARGS_LIST[3]);
	_tprintf(_T("%s %s /l\n"), example, argv[0]); 
	_tprintf(_T("To stop a running timer use %s, %s, %s, or %s\n"), ARGS_STOP[0], ARGS_STOP[1], ARGS_STOP[2], ARGS_STOP[3]);
	_tprintf(_T("%s %s /s <timer number>\n"), example, argv[0]);
	_tprintf(_T("To disable speech and only display the message box use %s\n"), ARG_MSG_ONLY);
	_tprintf(_T("%s %s 5 \"Coffee is ready.\" %s\n"), example, argv[0], ARG_MSG_ONLY);
	
	return;
}
