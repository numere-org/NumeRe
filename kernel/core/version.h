#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "31";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2020";
	static const char UBUNTU_VERSION_STYLE[] =  "20.10";
	
	//Software Status
	static const char STATUS[] =  "Release Candidate 3 HF1";
	static const char STATUS_SHORT[] =  "";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 2;
	static const long REVISION  = 663;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 4349;
	#define RC_FILEVERSION 1,1,2,663
	#define RC_FILEVERSION_STRING "1, 1, 2, 663\0"
	static const char FULLVERSION_STRING [] = "1.1.2.663";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
