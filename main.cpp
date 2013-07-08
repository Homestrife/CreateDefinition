//this code is just a temporary kludgy way to turn the contents of a directory into a fighter definition, assuming its contents are set up in a certain way.

#include "main.h"

int MakeHold(tinyxml2::XMLDocument * definition, tinyxml2::XMLElement * holds, string textureFilePath, int textureOffsetX, int textureOffsetY)
{
	//create the elements
	XMLElement * hold = definition->NewElement("Hold");
	hold->SetAttribute("id", curHold);
	hold->SetAttribute("duration", 4);
	XMLElement * textures = definition->NewElement("Textures");
	XMLElement * texture = definition->NewElement("Texture");
	texture->SetAttribute("textureFilePath", textureFilePath.data());
	texture->SetAttribute("offsetX", textureOffsetX);
	texture->SetAttribute("offsetY", textureOffsetY);

	//insert the elements
	holds->InsertEndChild(hold);
	hold->InsertEndChild(textures);
	textures->InsertEndChild(texture);

	//increment to the next hold id and return
	prevHold = hold;
	curHold++;
	return 0;
}

int HandleSubdirectory(HANDLE hFind, WIN32_FIND_DATA FindFileData, tinyxml2::XMLDocument * definition, tinyxml2::XMLElement * holds, tinyxml2::XMLElement * eventHolds, string directoryPath)
{
	//see if the file is a directory
	if(FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) { return 0; }

	//check the folder name
	string folderName = FindFileData.cFileName;
	if(folderName != "standing" && folderName != "turn" && folderName != "walking" && folderName != "running" && folderName != "runningTurn" && folderName != "runningStop"
		&& folderName != "crouch" && folderName != "crouching" && folderName != "crouchingTurn" && folderName != "stand" && folderName != "jumpNeutralStart"
		&& folderName != "jumpNeutralRising" && folderName != "jumpNeutralFall" && folderName != "jumpNeutralFalling" && folderName != "jumpNeutralLand"
		&& folderName != "jumpNeutralLandHard" && folderName != "airDashForward" && folderName != "airDashBackward")
	{ return 0; }

	//make a hold for each tga file in the folder
	WIN32_FIND_DATA FindTGAData;
	HANDLE tgaFind;
	string tgaDirectory = directoryPath + "\\" + folderName;
	string tgaSearchDirectory = tgaDirectory + "\\*.*";
	tgaFind = FindFirstFile(tgaSearchDirectory.data(), &FindTGAData);
	string filename = FindTGAData.cFileName;

	//get the hold attributes
	string attributesFilePath = tgaDirectory + "\\attributes.xml";
	tinyxml2::XMLDocument * attributesFile = new tinyxml2::XMLDocument();
	if(int error = attributesFile->LoadFile(attributesFilePath.data()) != 0)
	{
		return error; //couldn't load the file
	}
	if(strcmp(attributesFile->RootElement()->Value(), "Attributes") != 0)
	{
		return -1; //this isn't an attributes definition file
	}

	//get the attributes
	int textureOffsetX = 0;
	int textureOffsetY = 0;
	for(XMLElement * i = attributesFile->RootElement()->FirstChildElement(); i != NULL; i = i->NextSiblingElement())
	{
		string attributesType = i->Value();
		if(attributesType == "TextureAttributes")
		{
			i->QueryIntAttribute("offsetX", &(textureOffsetX));
			i->QueryIntAttribute("offsetY", &(textureOffsetY));
		}
	}
	
	bool firstHoldMade = false;
	if(filename.substr(filename.find_last_of(".") + 1) == "tga")
	{
		if(!firstHoldMade)
		{
			firstHoldMade = true;
			eventHolds->SetAttribute(folderName.data(), curHold);
		}
		else if(prevHold != NULL)
		{
			prevHold->SetAttribute("nextHold", curHold);
		}
		string textureFilePath = folderName + "\\" + filename;
		MakeHold(definition, holds, textureFilePath, textureOffsetX, textureOffsetY);
	}

	while(FindNextFile(tgaFind, &FindTGAData))
	{
		string filename = FindTGAData.cFileName;
		if(filename.substr(filename.find_last_of(".") + 1) == "tga")
		{
			if(!firstHoldMade)
			{
				firstHoldMade = true;
				eventHolds->SetAttribute(folderName.data(), curHold);
			}
			else if(prevHold != NULL)
			{
				prevHold->SetAttribute("nextHold", curHold);
			}
			string textureFilePath = folderName + "\\" + filename;
			MakeHold(definition, holds, textureFilePath, textureOffsetX, textureOffsetY);
		}
	}

	return 0;
}

int CreateDefinition( string directoryPath )
{
	//see if the given directory exists
    WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile(directoryPath.data(), &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE) { return -1; }
	string definitionName = FindFileData.cFileName;
	definitionName = directoryPath + "\\" + definitionName + ".xml";

	//create a definition file
	tinyxml2::XMLDocument * definition = new tinyxml2::XMLDocument();
	//tinyxml2::XMLDeclaration * declaration = definition->NewDeclaration("
	tinyxml2::XMLElement * HSObjects = definition->NewElement("HSObjects");
	tinyxml2::XMLElement * Fighter = definition->NewElement("Fighter");
	tinyxml2::XMLElement * Holds = definition->NewElement("Holds");
	tinyxml2::XMLElement * TerrainBoxes = definition->NewElement("TerrainBoxes");
	XMLElement * Box = definition->NewElement("Box");
	tinyxml2::XMLElement * EventHolds = definition->NewElement("EventHolds");
	definition->InsertEndChild(HSObjects);
	HSObjects->InsertEndChild(Fighter);
	Fighter->InsertEndChild(Holds);
	Fighter->InsertEndChild(TerrainBoxes);
	Fighter->InsertEndChild(EventHolds);
	TerrainBoxes->InsertEndChild(Box);

	//get the main attribute file
	string attributesFilePath = directoryPath + "\\attributes.xml";
	tinyxml2::XMLDocument * attributesFile = new tinyxml2::XMLDocument();
	if(int error = attributesFile->LoadFile(attributesFilePath.data()) != 0)
	{
		return error; //couldn't load the file
	}
	if(strcmp(attributesFile->RootElement()->Value(), "Attributes") != 0)
	{
		return -1; //this isn't an attributes definition file
	}

	//get the attributes
	for(XMLElement * i = attributesFile->RootElement()->FirstChildElement(); i != NULL; i = i->NextSiblingElement())
	{
		string attributesType = i->Value();
		if(attributesType == "FighterAttributes")
		{
			bool falls = true;
			float bounce = 0.8;
			float friction = 0.2;
			int walkSpeed = 0;
			int runSpeed = 0;
			int jumpSpeed = 0;
			int forwardAirDashSpeed = 0;
			int forwardAirDashDuration = 0;
			int backwardAirDashSpeed = 0;
			int backwardAirDashDuration = 0;
			int stepHeight = 10;
			float airControlAccel = 0;
			int maxAirControlSpeed = 0;
			string palette1FilePath = "";

			i->QueryIntAttribute("walkSpeed", &(walkSpeed));
			i->QueryIntAttribute("runSpeed", &(runSpeed));
			i->QueryIntAttribute("jumpSpeed", &(jumpSpeed));
			i->QueryIntAttribute("forwardAirDashSpeed", &(forwardAirDashSpeed));
			i->QueryIntAttribute("backwardAirDashSpeed", &(backwardAirDashSpeed));
			i->QueryIntAttribute("forwardAirDashDuration", &(forwardAirDashDuration));
			i->QueryIntAttribute("backwardAirDashDuration", &(backwardAirDashDuration));
			i->QueryIntAttribute("stepHeight", &(stepHeight));
			i->QueryFloatAttribute("airControlAccel", &(airControlAccel));
			i->QueryIntAttribute("maxAirControlSpeed", &(maxAirControlSpeed));
			palette1FilePath = i->Attribute("palette1FilePath");

			Fighter->SetAttribute("falls", falls);
			Fighter->SetAttribute("bounce", bounce);
			Fighter->SetAttribute("friction", friction);
			Fighter->SetAttribute("walkSpeed", walkSpeed);
			Fighter->SetAttribute("runSpeed", runSpeed);
			Fighter->SetAttribute("jumpSpeed", jumpSpeed);
			Fighter->SetAttribute("forwardAirDashSpeed", forwardAirDashSpeed);
			Fighter->SetAttribute("backwardAirDashSpeed", backwardAirDashSpeed);
			Fighter->SetAttribute("forwardAirDashDuration", forwardAirDashDuration);
			Fighter->SetAttribute("backwardAirDashDuration", backwardAirDashDuration);
			Fighter->SetAttribute("stepHeight", stepHeight);
			Fighter->SetAttribute("airControlAccel", airControlAccel);
			Fighter->SetAttribute("maxAirControlSpeed", maxAirControlSpeed);
			Fighter->SetAttribute("palette1FilePath", palette1FilePath.data());
		}
		else if(attributesType == "TerrainBoxAttributes")
		{
			int height = 0;
			int width = 0;
			int offsetX = 0;
			int offsetY = 0;
			
			i->QueryIntAttribute("height", &(height));
			i->QueryIntAttribute("width", &(width));
			i->QueryIntAttribute("offsetX", &(offsetX));
			i->QueryIntAttribute("offsetY", &(offsetY));

			Box->SetAttribute("height", height);
			Box->SetAttribute("width", width);
			Box->SetAttribute("offsetX", offsetX);
			Box->SetAttribute("offsetY", offsetY);
		}
	}

	//get all the subdirectories
	string directorySearchPath = directoryPath + "\\*.*";
	hFind = FindFirstFile(directorySearchPath.data(), &FindFileData);

	HandleSubdirectory(hFind, FindFileData, definition, Holds, EventHolds, directoryPath);
	
	while(FindNextFile(hFind, &FindFileData))
	{
		HandleSubdirectory(hFind, FindFileData, definition, Holds, EventHolds, directoryPath);
	}

	//save the definition file
	definition->SaveFile(definitionName.data());

	return 0;
}

//main
int main( int argc, char* argv[] )
{
	int error;
	if(argc > 1)
	{
		for(int i = 1; i < argc; i++)
		{
			error = CreateDefinition(argv[i]);
		}
	}

	CreateDefinition("C:\\Users\\Darlos9D\\Documents\\Visual Studio 2010\\Projects\\Homestrife\\Homestrife\\john");

	return 0;
}