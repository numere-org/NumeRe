#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{

	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2024";
	static const char UBUNTU_VERSION_STYLE[] =  "24.12";

	//Software Status
	static const char STATUS[] =  "Beta";
	static const char STATUS_SHORT[] =  "b";

	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 7;
	static const long REVISION  = 712;

	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 5152;
	#define RC_FILEVERSION 1,1,7,712
	#define RC_FILEVERSION_STRING "1, 1, 7, 712\0"
	static const char FULLVERSION_STRING [] = "1.1.7.712";

	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;

}
#endif //VERSION_H
