#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "24";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2019";
	static const char UBUNTU_VERSION_STYLE[] =  "19.09";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 1";
	static const char STATUS_SHORT[] =  "rc1";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 2;
	static const long REVISION  = 639;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4155;
	#define RC_FILEVERSION 1,1,2,639
	#define RC_FILEVERSION_STRING "1, 1, 2, 639\0"
	static const char FULLVERSION_STRING [] = "1.1.2.639";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
