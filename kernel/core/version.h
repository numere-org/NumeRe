#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{

	//Date Version Types
	static const char DATE[] = "26";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2025";
	static const char UBUNTU_VERSION_STYLE[] =  "25.04";

	//Software Status
	static const char STATUS[] =  "Release Candidate";
	static const char STATUS_SHORT[] =  "rc";

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
