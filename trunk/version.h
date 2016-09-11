#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "10";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.09";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 3";
	static const char STATUS_SHORT[] =  "rc3";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 0;
	static const long BUILD  = 9;
	static const long REVISION  = 93;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2736;
	#define RC_FILEVERSION 1,0,9,93
	#define RC_FILEVERSION_STRING "1, 0, 9, 93\0"
	static const char FULLVERSION_STRING [] = "1.0.9.93";
	
	//SVN Version
	static const char SVN_REVISION[] = "33";
	static const char SVN_DATE[] = "2016-08-10T04:58:49.492466Z";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 58;
	

}
#endif //VERSION_H
