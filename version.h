#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "12";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.04";
	
	//Software Status
	static const char STATUS[] =  "Bloch";
	static const char STATUS_SHORT[] =  "b";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 0;
	static const long BUILD  = 8;
	static const long REVISION  = 58;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2700;
	#define RC_FILEVERSION 1,0,8,58
	#define RC_FILEVERSION_STRING "1, 0, 8, 58\0"
	static const char FULLVERSION_STRING [] = "1.0.8.58";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 58;
	

}
#endif //VERSION_H
