/*******************\
*Permissions Manager*
\*******************/
//Source code for the permissions manager header file, created for the
//Chameleon Flex IDE.
//Author: Ben Carhart
//Date started: 11/22/03
//Date officially completed: --/--/--
//Description: manages the handling of permissions
//Major revisions:
//  ~BSC--xx/xx/xx--Changed the loading of the permCode by using a temp
//                  bitset variable
//  ~BSC--xx/xx/xx--Altered the error handling of the program to be
//                  a little more graceful.
//  ~BSC--11/29/03--Added the authorization bitset variable & implementation
//  ~BSC--02/09/04--Removed all file access.  User must pass in the # on
//                  initialization.  Also # encryption removed.
//  ~BSC--02/26/04--Changed authorization code to include check-sum for some
//                  security against kids just randomly changing authorization
//                  numbers and getting interesting authorized results.  i
//                  also pulled out the little extra code-checking down in
//                  "setGlobal()".  Why?  It wasn't working right.
//  ~BSC--03/20/04--adjusted authorization so that if "adv. comp." is enabled,
//					so is "compilation".  I basically put a small "if()...then"
//                  statement everywhere the [permCode] is somehow modified.  I
//                  also set it so that if normal compilation is disabled*, ADV
//                  compilation is disabled.  I didn't touch authorizations.
//                  *note- this disabling only occurs of "disable(id)" is called.
//  ~BSC--03/22/04--Adjused for Mark: a blank constructor
//  ~BSC--04/23/04--blank constructor removed!!  why?  oh we changed things  ^_^
//                  but this class is essentially done.  I think it can be ignored
//                  if other items are added later, but i'm not fully sure.  Mark
//                  and Dave are responsible for adding items, so I never saw what
//                  Mark had to do per-item-added.

//includes
#include "p.h"
#include "../common/Crc16.h"
#include "../common/debug.h"

//global variables
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

wxString GlobalPermStrings[] = {"Syntax highlighting",
								"Auto-indentation",
								"Debugging",
								"Terminal",
								"Local mode",
								"Projects",
								"Compilation",
								//"Advanced compiler output",
								//"Test permission"
								};

//@@@@@@@@@
//@ BEGIN @
//@@@@@@@@@

//Constructor(s)

//////////////////////////////////////////////////////////////////////////////
///  public constructor Permission
///
///  @param  loadAuthCode wxString  [="0"] <initial authorization code>
///  @param  loadPermCode wxString  [="0"] <initial "things active" code>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
Permission::Permission(wxString loadAuthCode, wxString loadPermCode)
{
	long tPCode, tACode;

	loadPermCode.ToLong(&tPCode);
	loadAuthCode.ToLong(&tACode);

	for(int i = 0; i < PERM_LAST; i++)
	{
		permNames.Add(GlobalPermStrings[i]);
	}

	//load permission code into bitset Array

	bitset<NUM_MODULES> tempP(tPCode);
	bitset<NUM_MODULES> tempA(tACode);
	status = tempP;
	auth = tempA;

	permCode = status.to_ulong();
	authCode = auth.to_ulong();
}

//Destructor
//Kills everything.
//////////////////////////////////////////////////////////////////////////////
///  public destructor ~Permission
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
Permission::~Permission()
{
	status.reset();
	auth.reset();
	permCode = 0;
	authCode = 0;
}

//////////////////////////////////////////////////////////////////////////////
///  public isEnabled
///  <answers the question "is this item ACTIVE and AUTHORIZED?">
///
///  @param  id   int  <one of the enums in "datastructures.h">
///
///  @return bool <the answer>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Permission::isEnabled(int id)
{
	return(status.test(id) && auth.test(id));
}

//////////////////////////////////////////////////////////////////////////////
///  public isAuthorized
///  <answers the question "is this item AUTHORIZED?">
///
///  @param  id   int  <a module ID from the enum in "datastructures.h">
///
///  @return bool <the answer>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Permission::isAuthorized(int id)
{
	return(auth.test(id));
}

//////////////////////////////////////////////////////////////////////////////
///  public enable
///  <enables a given module ID if authorized as well>
///
///  @param  id   int  <module ID>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Permission::enable(int id)
{
	if(isAuthorized(id))
	{
		status.set(id);
		permCode = status.to_ulong();
	}

}

//////////////////////////////////////////////////////////////////////////////
///  public disable
///  <disbles a given module ID (authorization not required)>
///
///  @param  id   int  <module ID>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Permission::disable(int id)
{
	status.reset(id);
	permCode = status.to_ulong();
}

//////////////////////////////////////////////////////////////////////////////
///  public setGlobalAuthorized
///  <sets a global authorization code>
///
///  @param  newAuthCode wxString  <a valid CRC string>
///
///  @return bool        <successful authorization>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Permission::setGlobalAuthorized(wxString newAuthCode)
{
	wxString crcPrefix;
	wxString crcGenerated;
	wxString numberString;

	if(newAuthCode.Len() < 4)
	{
		return false;
	}

	newAuthCode.MakeUpper();
	crcPrefix = newAuthCode.Left(4);

	numberString = newAuthCode.Mid(4);
	//wxLogDebug("newAuthCode: %s", newAuthCode);
	//wxLogDebug("numberString: %s", numberString);
	//wxLogDebug("crcPrefix: %s", crcPrefix);

	ClsCrc16 crc;
	crc.CrcInitialize();
	crc.CrcAdd(numberString.c_str(), numberString.Len());

	crcGenerated.Printf("%X", crc.CrcGet());
	//wxLogDebug("crcGenerated: %s", crcGenerated);

	bool validAuthCode = (crcGenerated == crcPrefix);

	if(validAuthCode)
	{
		numberString.ToLong(&authCode);
		auth = bitset<NUM_MODULES>(authCode);
		savedAuthCode = newAuthCode;
	}

	return validAuthCode;
}

//////////////////////////////////////////////////////////////////////////////
///  public setGlobalEnabled
///  <sets a global "this item is active" code>
///
///  @param  newEnableCode wxString  <a valid enable code>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Permission::setGlobalEnabled(wxString newEnableCode)
{
	newEnableCode.ToLong(&permCode);
	status = bitset<NUM_MODULES>(permCode);

	savedPermCode << getGlobalEnabled();
}

//////////////////////////////////////////////////////////////////////////////
///  public getGlobalEnabled
///  <returns the current bit-for-bit enabled items>
///
///  @return long <the integer "this item is active" code>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
long Permission::getGlobalEnabled()
{
	return(permCode);
}

//////////////////////////////////////////////////////////////////////////////
///  public getGlobalAuthorized
///  <returns the bit-for-bit authorized items>
///
///  @return long <the integer authorization code>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
long Permission::getGlobalAuthorized()
{
	return(authCode);
}


//////////////////////////////////////////////////////////////////////////////
///  public getPermName
///  <a function mark uses to enumerate the permissions items in the GUI>
///
///  @param  permEnum int  <the particular "name" to get>
///
///  @return wxString <the "name" gotten>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString Permission::getPermName(int permEnum)
{

	if((int)permNames.Count() > permEnum)
	{
		return permNames.Item(permEnum);
	}
	return wxEmptyString;
}
//@@@@@@@
//@ END @
//@@@@@@@
