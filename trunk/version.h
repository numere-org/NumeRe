#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "26";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.09";
	
	//Software Status
	static const char STATUS[] =  "Bethe";
	static const char STATUS_SHORT[] =  "b";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 0;
	static const long BUILD  = 9;
	static const long REVISION  = 93;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2742;
	#define RC_FILEVERSION 1,0,9,93
	#define RC_FILEVERSION_STRING "1, 0, 9, 93\0"
	static const char FULLVERSION_STRING [] = "1.0.9.93";
	
	//SVN Version
	static const char SVN_REVISION[] = "41";
	static const char SVN_DATE[] = "2016-09-26T18:35:00.818734Z";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 58;
	

}
#endif //VERSION_H
