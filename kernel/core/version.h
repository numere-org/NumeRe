#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{

	//Date Version Types
	static const char DATE[] = "16";
	static const char MONTH[] = "05";
	static const char YEAR[] = "2023";
	static const char UBUNTU_VERSION_STYLE[] =  "23.05";

	//Software Status
	static const char STATUS[] =  "Cherenkov";
	static const char STATUS_SHORT[] =  "";

	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 5;
	static const long REVISION  = 700;

	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4983;
	#define RC_FILEVERSION 1,1,5,700
	#define RC_FILEVERSION_STRING "1, 1, 5, 700\0"
	static const char FULLVERSION_STRING [] = "1.1.5.700";

	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;

}
#endif //VERSION_H
