#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "12";
	static const char MONTH[] = "05";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.05";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 1";
	static const char STATUS_SHORT[] =  "rc1";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 0;
	static const long BUILD  = 9;
	static const long REVISION  = 73;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2700;
	#define RC_FILEVERSION 1,0,9,73
	#define RC_FILEVERSION_STRING "1, 0, 9, 73\0"
	static const char FULLVERSION_STRING [] = "1.0.9.73";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 58;
	

}
#endif //VERSION_H
