// #define UNICODE /* defined automatically by the -municode flag in gcc */
#define _UNICODE
#define _WIN32_WINNT 0x0400
#define _WIN32_DCOM

#include <initguid.h>
#include <windows.h>
#include <sapi.h>
#include <ole2.h>
#include <oleauto.h>
#include <wbemidl.h>
#include <tchar.h>
#include <Tlhelp32.h>
#include <stdbool.h>

#define NAME _T("TimeMe")
#define VERSION _T(" v1.0")
#define TITLE NAME VERSION
#define LIST_ARGS_COUNT 4
#define STOP_ARGS_COUNT 4
#define ARG_SET _T("/set")
#define ARG_SET_MSG_ONLY _T("/setm")
#define ARG_MSG_ONLY _T("/m")
#define ARG_OPTIONS _T("/?")
#define SLASH _T('/')
#define DASH _T('-')
#define SPEAK_PAUSE 250
#define MAX_MINUTES 525960 // Average number of minutes in a year.

#define HELP 0
#define SET 1
#define SET_MSG_ONLY 2
#define START 3
#define LIST 4
#define STOP 5
#define OPTIONS 6
#define ARG_ERR 7

const TCHAR TIME_ERROR_MSG[] = 
	_T("Time has to be between 0 and %d.\n");
	
const TCHAR *ARGS_LIST[LIST_ARGS_COUNT] =	{
	
	_T("/list"), 
	_T("/l"), 
	_T("-list"), 
	_T("-l")				
											};
											
const TCHAR *ARGS_STOP[STOP_ARGS_COUNT] =	{
	
	_T("/stop"), 
	_T("/s"), 
	_T("-stop"), 
	_T("-s")				
											};

struct THREAD_MSG {
  TCHAR * msg;
  bool thread_running;
};
typedef struct THREAD_MSG THREAD_MSG;

char Check_Arguments(int argc, _TCHAR * argv[]);
void Set_Timer(int argc, _TCHAR * argv[], bool msgonly);
bool Create_Timer(TCHAR cmdline[]);
void Start_Timer(float time, TCHAR msg[], bool include_voice);
DWORD WINAPI Show_Message(LPVOID lpParam);
bool List_Timers(_TCHAR * argv[], bool kill, char timer_to_kill, DWORD * ttk_pid);						
bool Get_Image_Name(TCHAR * image_name);
void Print_Timer_Info(BSTR timer_cmd_line, int timer_number, DWORD pid);
float Calculate_Time_Left(DWORD pid, float time_in_min);
void Stop_Timer(DWORD pid, char tmr_num);
void Print_Help(_TCHAR * argv[]);
void Print_Options(_TCHAR * argv[]);

/* Caller table:
 * -------------------------------------------
 * Function:				Caller:			 *
 * -------------------------------------------
 * Check_Arguments()		main()
 * Set_Timer()				main()
 * Create_Timer()			Set_Timer()
 * Start_Timer()			main()
 * Show_Message()			Start_Timer()
 * List_Timers()			main()
 * Get_Image_Name()			List_Timers()
 * Print_Timer_Info()		List_Timers()
 * Calculate_Time_Left()	Print_Timer_Info()
 * Stop_Timer()				main()
 * Print_Help()				main()
 * Print_Options()			main() 			 */
