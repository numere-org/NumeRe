#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{

	//Date Version Types
	static const char DATE[] = "24";
	static const char MONTH[] = "03";
	static const char YEAR[] = "2022";
	static const char UBUNTU_VERSION_STYLE[] =  "22.03";

	//Software Status
	static const char STATUS[] =  "Release Candidate";
	static const char STATUS_SHORT[] =  "rc";

	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 4;
	static const long REVISION  = 691;

	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4791;
	#define RC_FILEVERSION 1,1,4,691
	#define RC_FILEVERSION_STRING "1, 1, 4, 691\0"
	static const char FULLVERSION_STRING [] = "1.1.4.691";

	//SVN Version
	static const char SVN_REVISION[] = "1065";
	static const char SVN_DATE[] = "2022-02-09T17:25:42.137372Z";

	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;

}
#endif //VERSION_H
