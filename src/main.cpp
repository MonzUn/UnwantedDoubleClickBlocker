#include <iostream>
#include <chrono>
#include <interception.h>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define MINIMUM_CLICK_DELTA_TIME 40

int main()
{
	InterceptionContext context;
	InterceptionDevice	device;

	SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS ); // Ensures that we receive the input before the applications we want to protect against faulty double clicks

	context = interception_create_context();

	InterceptionFilter filter = INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN | INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP;
	interception_set_filter( context, interception_is_mouse, filter );

	InterceptionMouseStroke stroke;
	std::chrono::steady_clock::time_point lastRelease = std::chrono::high_resolution_clock::now();
	bool expectingRelease = false;
	while( interception_receive(context, device = interception_wait( context ), reinterpret_cast<InterceptionStroke*>( &stroke ), 1) > 0 ) // Blocks until a stroke with the set filter is received. Continues as long as no error occurs.
	{
		bool shouldForward = false;
		if ( !expectingRelease && stroke.state & INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN )
		{
			unsigned int timeSinceLastRelease = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now() - lastRelease ).count();
			if ( timeSinceLastRelease >= MINIMUM_CLICK_DELTA_TIME )
			{
				shouldForward		= true;
				expectingRelease	= true;
			} else
				std::cout << "A click was blocked. Delta time from last release = " << timeSinceLastRelease << "ms\n";
		}
		else if( expectingRelease && stroke.state & INTERCEPTION_MOUSE_LEFT_BUTTON_UP )
		{
			lastRelease			= std::chrono::high_resolution_clock::now();
			expectingRelease	= false;
			shouldForward		= true;
		}

		if ( shouldForward )
			interception_send( context, device, reinterpret_cast< InterceptionStroke* >( &stroke ), 1 );
	}

	return 0;
}