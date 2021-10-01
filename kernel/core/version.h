#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "30";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2021";
	static const char UBUNTU_VERSION_STYLE[] =  "21.09";
	
	//Software Status
	static const char STATUS[] =  "Marie Curie HF1";
	static const char STATUS_SHORT[] =  "hf1";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 3;
	static const long REVISION  = 689;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4525;
	#define RC_FILEVERSION 1,1,3,689
	#define RC_FILEVERSION_STRING "1, 1, 3, 689\0"
	static const char FULLVERSION_STRING [] = "1.1.3.689";
	
	//SVN Version
	static const char SVN_REVISION[] = "1017";
	static const char SVN_DATE[] = "2021-09-29T19:16:12.474917Z";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
