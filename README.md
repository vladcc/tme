# tme
A countdown timer that won't shut up until you press OK (yes, it talks)

tme (short for time me) is a little cmd Windows program that let's you countdown a certain amount of time and tells you when
the time is up. What makes this one interesting is that it talks to you. It goes like this:

1. You set up a timer with a message
2. The time passes
3. A message box which doesn't go away util you click it appears on your screen and a voice reads the message to you
4. You press OK

I made this because I always used to forget the coffee on the stove. Now I don't anymore. Works like magic.

What's technically interesting about tme is that when you set a timer the process recursively starts itself and sleeps. 
This allows it to sit in memory and report how much time is left, what's the message you've set it with and so on. When the time
is up another thread is started to display the message box while the default thread reads the message. At any one point you have the
option to list all available timers in memory and stop them. The project utilizes COM objects for communication with the Windows voice
synthesizer and WMI. Only plain C is used.

In case you are wondering after seeing the code, at the time of writing this I was reading Assemble Language Step-by-Step, so I went
totally overboard with the documentation. Great book by the way.

Try it and let me know what you think.
Oh, and don't prank nobody :)
