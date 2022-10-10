#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2022";
	static const char UBUNTU_VERSION_STYLE[] =  "22.09";

	//Software Status
	static const char STATUS[] =  "Compton";
	static const char STATUS_SHORT[] =  "";

	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 4;
	static const long REVISION  = 691;

	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4950;
	#define RC_FILEVERSION 1,1,4,691
	#define RC_FILEVERSION_STRING "1, 1, 4, 691\0"
	static const char FULLVERSION_STRING [] = "1.1.4.691";

	//SVN Version
	static const char SVN_REVISION[] = "1218";
	static const char SVN_DATE[] = "2022-09-19T20:24:53.239714Z";

	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;

}
#endif //VERSION_H
