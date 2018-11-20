#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "11";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.11";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 3";
	static const char STATUS_SHORT[] =  "rc3";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 1;
	static const long REVISION  = 418;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 3921;
	#define RC_FILEVERSION 1,1,1,418
	#define RC_FILEVERSION_STRING "1, 1, 1, 418\0"
	static const char FULLVERSION_STRING [] = "1.1.1.418";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
