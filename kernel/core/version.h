#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "10";
	static const char MONTH[] = "03";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.03";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 1";
	static const char STATUS_SHORT[] =  "rc1";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 1;
	static const long REVISION  = 284;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 3832;
	#define RC_FILEVERSION 1,1,1,284
	#define RC_FILEVERSION_STRING "1, 1, 1, 284\0"
	static const char FULLVERSION_STRING [] = "1.1.1.284";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
