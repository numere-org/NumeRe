#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{

	//Date Version Types
	static const char DATE[] = "16";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2026";
	static const char UBUNTU_VERSION_STYLE[] =  "26.09";

	//Software Status
	static const char STATUS[] =  "Chamberlain";
	static const char STATUS_SHORT[] =  "";

	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 8;
	static const long REVISION  = 867;

	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 5393;
	#define RC_FILEVERSION 1,1,8,867
	#define RC_FILEVERSION_STRING "1, 1, 8, 867\0"
	static const char FULLVERSION_STRING [] = "1.1.8.867";

	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;

}
#endif //VERSION_H
