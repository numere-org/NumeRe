#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "11";
	static const char MONTH[] = "05";
	static const char YEAR[] = "2020";
	static const char UBUNTU_VERSION_STYLE[] =  "20.05";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 3";
	static const char STATUS_SHORT[] =  "rc3";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 2;
	static const long REVISION  = 658;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4201;
	#define RC_FILEVERSION 1,1,2,658
	#define RC_FILEVERSION_STRING "1, 1, 2, 658\0"
	static const char FULLVERSION_STRING [] = "1.1.2.658";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
