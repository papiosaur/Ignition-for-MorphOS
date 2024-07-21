/* Version numbers
 *
 * Copyright ©2005-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef VERSION_H
#define VERSION_H


#ifdef __amigaos4__
	#define VERSION "1.22"
	#define INFODATE "28. Juli 2023" 
	#define IGNITION_COPYRIGHT "Copyright ©1996-2023 pinc Software."
	#define VERSTRING "$VER: ignitionOS4 " VERSION " (28.07.2023)"
 
	#define SCREEN_TITLE "ignitionMOS  V" VERSION "    "  IGNITION_COPYRIGHT							
#else

	#define SCREEN_TITLE "ignition"

	#define VERSION "1.3 beta 1"
	#define INFODATE "21. Julio 2024"
	#define IGNITION_COPYRIGHT "Copyright ©1996-2024 pinc Software."
	#define VERSTRING "$VER: ignition " VERSION " (21.7.2024)"
#endif
#endif	/* VERSION_H */

