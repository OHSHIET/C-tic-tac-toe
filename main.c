#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void ClearScreen()
{
  HANDLE                     hStdOut;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD                      count;
  DWORD                      cellCount;
  COORD                      homeCoords = { 0, 0 };

  hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
  if (hStdOut == INVALID_HANDLE_VALUE) return;

  /* Get the number of cells in the current buffer */
  if (!GetConsoleScreenBufferInfo( hStdOut, &csbi )) return;
  cellCount = csbi.dwSize.X *csbi.dwSize.Y;

  /* Fill the entire buffer with spaces */
  if (!FillConsoleOutputCharacter(
    hStdOut,
    (TCHAR) ' ',
    cellCount,
    homeCoords,
    &count
    )) return;

  /* Fill the entire buffer with the current colors and attributes */
  if (!FillConsoleOutputAttribute(
    hStdOut,
    csbi.wAttributes,
    cellCount,
    homeCoords,
    &count
    )) return;

  /* Move the cursor home */
  SetConsoleCursorPosition( hStdOut, homeCoords );
}

#else // !_WIN32
#include <unistd.h>
#include <term.h>

void ClearScreen()
{
  if (!cur_term)
  {
     int result;
     setupterm( NULL, STDOUT_FILENO, &result );
     if (result <= 0) return;
  }

   putp( tigetstr( "clear" ) );
}
#endif

const unsigned short matrixWidth = 20, matrixHeight = 7;

const char matrixGlobal[7][20] = {
  {' ', ' ', ' ', ' ', ' ', ' ', '1', ' ', '2', ' ', '3', ' ', '4', ' ', '5', ' ', '6', ' ', '7', '\n'},
  {' ', ' ', ' ', ' ', '1', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', '\n'},
  {' ', ' ', ' ', ' ', '2', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', '\n'},
  {' ', ' ', ' ', ' ', '3', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', '\n'},
  {' ', ' ', ' ', ' ', '4', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', '\n'},
  {' ', ' ', ' ', ' ', '5', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', '\n'},
  {' ', ' ', ' ', ' ', '6', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', ' ', '*', '\n'},
};

const unsigned short maxPlayerNameLength = 16;

bool notACorrectOption;

typedef struct {
  char board[6][7], firstPlayerName[16], secondPlayerName[16], playerSaveName[16], oneDimensionalBoard[42], oneLine[100];
  unsigned long gameID, playerOccurrence;
  unsigned short emptyFields;
} gameSave;

void mainMenu();

void newGame();

void loadSavedGameMenu();

void listSavedGames();

void listSavedGamesForPlayer();

void showBoardForSavedGame();

void loadSavedGame();

void startAGame(char*, char*, char[42]);

void saveTheGame(char*, char*, unsigned short, gameSave);

char *inputString(FILE*, size_t);

void checkSaveFile(FILE*);

int maxBetween2Ints(int, int);

int main()
{
    mainMenu();
    return 0;
}

void mainMenu() {
    ClearScreen();
    printf("For input, type in the number corresponding with the option you want to choose.\n");
    printf("1. Play a new game.\n");
    printf("2. Load already saved game.\n");
    printf("3. Exit the game.\n");
    int mainMenuNum;
    do {
        notACorrectOption = false;
        scanf("%d", &mainMenuNum);
        fflush(stdin);
        switch (mainMenuNum) {
        case 1:
            newGame();
            break;
        case 2:
            loadSavedGameMenu();
            break;
        case 3:
            printf("\nExiting the game...\n");
            exit(1);
        default:
            notACorrectOption = true;
            printf("Not a correct option.\n");
            break;
        }
    } while (notACorrectOption);
}

void startAGame(char *firstPlayerName, char *secondPlayerName, char board[42]){
    ClearScreen();

    unsigned short boardCounter = 0;
    unsigned short XCounter = 0, OCounter = 0;
    char matrix[matrixHeight][matrixWidth];

    // handle whether the game is new or loaded from a save
    for(unsigned short i = 0; i < matrixHeight; i++){
      for(unsigned short j = 0; j < matrixWidth; j++){
        if(board[0] == 0)
          matrix[i][j] = matrixGlobal[i][j];
        else{
          if(i >= 1 && j >= 6 && j <= matrixWidth - 2 && !(j % 2)){
            matrix[i][j] = board[boardCounter++];
            if(matrix[i][j] == 'X') XCounter++;
            else if(matrix[i][j] == 'O') OCounter++;
          }
          else if(i != 0){
            matrix[i][j] = (j == matrixWidth - 1) ? '\n' : ((j == 4) ? (char) i + '0' : ' ');
          }
          else {
            matrix[i][j] = (j >= 6 && j <= matrixWidth - 2 && !(j%2)) ? (char) (j / 2 - 2) + '0' : (j == matrixWidth - 1) ? '\n' : ' ';
          }
        }
      }
    }

    void printBoard(bool printDoubleNewLine){
      if(printDoubleNewLine) printf("\n");
      printf("\n");
      for (unsigned short i = 0; i < matrixHeight; i++) {
          for(unsigned short j = 0; j < matrixWidth; j++){
              printf("%c", matrix[i][j]);
          }
      }
    }

    void printMainInfo(){
      printf("%s is playing as X.\n", firstPlayerName);
      printf("%s is playing as O.\n\n", secondPlayerName);
      printf("To play, type in 2 numbers: first corresponding with the row: [1-6], second with the column: [1-7]. Entering 0 saves the game, -1 clears the console, -2 exits the game to main menu.\n");
      printBoard(false);
    }

    gameSave matrixToSave;
    unsigned short gameHasntEnded = true, emptyCells, emptyCellsTemp = 42;
    char XOChar = (OCounter < XCounter) ? 'O' : 'X'; // determine whose turn it is after loading the game based on quantity of Xs and Os
    char whoseMove[maxBetween2Ints(strlen(firstPlayerName), strlen(secondPlayerName))];
    strcpy(whoseMove, (OCounter < XCounter) ? secondPlayerName : firstPlayerName);

    void gameEnd(unsigned short *gameHasntEndedArg, char winCase, unsigned short startingIndexI, unsigned short startingIndexJ, bool draw){
      *gameHasntEndedArg = false;
      switch(draw){
        case true:
        printf("\nDraw. Nobody won.\n");
        break;
        case false:
        printf("\nPlayer %s (%c) has won.", whoseMove, XOChar);
        short ki = startingIndexI;
        // sets the arrows to point at winner's fields
        switch(winCase){
          case 'h':
            for(unsigned short k = startingIndexJ; k <= startingIndexJ + 6; k += 2){
              matrix[startingIndexI][k - 1] = '>';
            }
            break;
          case 'v':
            for(unsigned short k = startingIndexI; k <= startingIndexI + 3; k++){
              matrix[k][startingIndexJ - 1] = '>';
            }
            break;
          case 'd':
            for(unsigned short kj = startingIndexJ; kj <= startingIndexJ + 6; kj += 2){
                matrix[ki++][kj - 1] = '>';
            }
            break;
          case 'b':
            for(short kj = startingIndexJ; kj >= startingIndexJ - 6; kj -= 2){
                matrix[ki++][kj - 1] = '>';
            }
            break;
        }
        break;
      }
      printBoard(true);
      int afterGameChoice;
      printf("\nWould you like to start a new game or go back to the main menu? 1 - new game, 2 - main menu.\n");
      char boardToPass[42] = {0};
      do {
        notACorrectOption = false;
        scanf("%d", &afterGameChoice);
        fflush(stdin);
        switch(afterGameChoice){
          case 1:
          startAGame(firstPlayerName, secondPlayerName, boardToPass);
          break;
          case 2:
          mainMenu();
          break;
          default:
          printf("\nNot a correct option.\n");
          notACorrectOption = true;
          break;
        }
      } while(notACorrectOption);
    }

    printMainInfo();

    while(gameHasntEnded){
        int playerMoveY, playerMoveX;
        emptyCells = 42;

        printf("\n%s's (%c) move.\n", whoseMove, XOChar);
        scanf("%d %d", &playerMoveY, &playerMoveX);
        fflush(stdin);
        char *characterToPlace = &matrix[playerMoveY][playerMoveX * 2 + 4];
        if(!playerMoveY || !playerMoveX){
          if(emptyCellsTemp == 42) {
            printf("\nSaving empty games is not allowed.\n");
            printBoard(false);
            continue;
          }
          int areYouSure;
          printf("Save the game?\n");
          printf("1 - proceed. 0 - back.\n");
          scanf("%d", &areYouSure);
          fflush(stdin);
          switch(areYouSure){
            case 1:
              saveTheGame(firstPlayerName, secondPlayerName, emptyCellsTemp, matrixToSave);
              printBoard(false);
            case 0: default:
              continue;
          }
        }
        else if(playerMoveX == -2 || playerMoveY == -2){
          int areYouSure;
          printf("Exit the game to main menu?\n");
          printf("1 - proceed. 0 - back.\n");
          scanf("%d", &areYouSure);
          fflush(stdin);
          switch(areYouSure){
            case 1:
              ClearScreen();
              printf("Exited the game.\n");
              mainMenu();
              break;
            case 0:
              continue;
          }
        }
        else if(playerMoveX == -1 || playerMoveY == -1){
          ClearScreen();
          printMainInfo();
          continue;
        }
        else if(playerMoveX < -2 || playerMoveX > 7 || playerMoveY < -2 || playerMoveY > 6 || *characterToPlace == 'X' || *characterToPlace == 'O'){
          printf("Invalid move.\n");
          continue;
        }

        *characterToPlace = XOChar; // set X or O on the board if everything is fine
        printBoard(false);

        // goes through the fields where X, O or * are located every turn.
        for (unsigned short i = 1; i < matrixHeight; i++) {
            for(unsigned short j = 6; j <= matrixWidth - 2; j += 2){
                // wins if horizontal
                if(j <= 12 && matrix[i][j] == XOChar && matrix[i][j + 2] == XOChar && matrix[i][j + 4] == XOChar && matrix[i][j + 6] == XOChar){
                  gameEnd(&gameHasntEnded, 'h', i, j, false);
                }
                // wins if vertical
                else if(i < matrixHeight - 3 && matrix[i][j] == XOChar && matrix[i + 1][j] == XOChar && matrix[i + 2][j] == XOChar && matrix[i + 3][j] == XOChar){
                  gameEnd(&gameHasntEnded, 'v', i, j, false);
                }
                // wins if diagonal
                else if(i < matrixHeight - 3 && j <= 12 && matrix[i][j] == XOChar && matrix[i + 1][j + 2] == XOChar && matrix[i + 2][j + 4] == XOChar && matrix[i + 3][j + 6] == XOChar){
                  gameEnd(&gameHasntEnded, 'd', i, j, false);
                }
                // wins if backwards diagonal
                else if(i < matrixHeight - 3 && j >= 12 && matrix[i][j] == XOChar && matrix[i + 1][j - 2] == XOChar && matrix[i + 2][j - 4] == XOChar && matrix[i + 3][j - 6] == XOChar){
                  gameEnd(&gameHasntEnded, 'b', i, j, false);
                }

                if(matrix[i][j] == 'X' || matrix[i][j] == 'O') emptyCellsTemp = --emptyCells;

                matrixToSave.board[i - 1][(j / 2) - 3] = matrix[i][j]; // board to be passed to the function if the players decide to save the game
            }
        }

        if(!emptyCells) gameEnd(&gameHasntEnded, 0, 0, 0, true); // draw

        // switch turns
        switch(XOChar){
          case 'X':
            strcpy(whoseMove, secondPlayerName);
            XOChar = 'O';
            break;
          case 'O':
            strcpy(whoseMove, firstPlayerName);
            XOChar = 'X';
            break;
        }
    }
}

void newGame() {
    char firstPlayerName[maxPlayerNameLength], secondPlayerName[maxPlayerNameLength];

    void readPlayerNames(char *currentPlayerName, bool isSecondPlayer){
        do {
            notACorrectOption = false;
            currentPlayerName = inputString(stdin, maxPlayerNameLength - 1);
            fflush(stdin);
            if(strlen(currentPlayerName) > maxPlayerNameLength - 1) {
                notACorrectOption = true;
                printf("Player names should not be longer than %u symbols.\n", maxPlayerNameLength - 1);
            }
            else if(isSecondPlayer && !strcmp(currentPlayerName, firstPlayerName)){
                notACorrectOption = true;
                printf("Player names should not be equal to each other.\n");
            }
            else {
              strcpy((isSecondPlayer) ? secondPlayerName : firstPlayerName, currentPlayerName);
            }
        } while(notACorrectOption);
        free(currentPlayerName);
    }

    printf("\nEnter the first player's name.\n");
    readPlayerNames(firstPlayerName, false);
    printf("\nEnter the second player's name.\n");
    readPlayerNames(secondPlayerName, true);

    char boardToPass[42] = {0};
    startAGame(firstPlayerName, secondPlayerName, boardToPass);
};

// checks if save file cannot be opened for some reason
void checkSaveFile(FILE *saveFile){
  if(saveFile == NULL){
    printf("\nError opening the file with save data or the file doesn't exist.\n");
    loadSavedGameMenu();
    return;
  }
  int getCharFromSaveFile = fgetc(saveFile);
  if(getCharFromSaveFile == EOF){
    printf("\nNo saves found. Returning to the menu.\n");
    loadSavedGameMenu();
    return;
  }
  rewind(saveFile); // move to beginning of file
}

void saveTheGame(char *firstPlayerName, char *secondPlayerName, unsigned short emptyCells, gameSave gameBoard){
  time_t secondsSave = time(NULL);
  gameSave save;
  save.gameID = (unsigned long) secondsSave; // save the game with current time, then convert it to hex
  strcpy(save.firstPlayerName, firstPlayerName);
  strcpy(save.secondPlayerName, secondPlayerName);
  save.emptyFields = emptyCells;

  FILE *saveFile, *saveFileTemp;
  saveFile = fopen("result.txt", "r");
  saveFileTemp = fopen("temp.txt", "a");
  if(saveFile == NULL){
      fclose(saveFile);
      // try to create a file if it doesnt exist; if there still is an error, display a message.
      saveFile = fopen("result.txt", "w");
      if(saveFile == NULL){
        printf("\nThere was an error opening the file with save data.\n");
        return;
      }
      fclose(saveFile);
      saveFile = fopen("result.txt", "r");
  }
  if(saveFileTemp == NULL){
    printf("\nThere was an error opening the file with save data.\n");
    return;
  }

  bool saveFileIsNotEmpty = (fgetc(saveFile) != EOF);

  // check how many unique player names there are in the database, to know how long the array should be
  unsigned long uniquePlayerNames = 0, playerNameCounter = 0;
  if(saveFileIsNotEmpty){
    rewind(saveFile); // move file pointer to the beginning
    while(fgets(save.oneLine, sizeof save.oneLine, saveFile)[0] == '0'){
      uniquePlayerNames++;
    }
  }

  rewind(saveFile); // move file pointer to the beginning again

  gameSave *savesForParticularPlayer = malloc(uniquePlayerNames * sizeof *savesForParticularPlayer);
  char _unused[16];


  while(fgets(save.oneLine, sizeof save.oneLine, saveFile) != NULL){
    if(save.oneLine[0] == '0'){
      // write info about the unique player names and how many games the players have totally played into the dynamically created array of structures
      sscanf(save.oneLine, "%x\t\t%lu\t\t%[^\t]\t\t%[^\t]\t\t%*s", &_unused, &save.playerOccurrence, save.playerSaveName, _unused, _unused);
      strcpy(savesForParticularPlayer[playerNameCounter].playerSaveName, save.playerSaveName);
      savesForParticularPlayer[playerNameCounter].playerOccurrence = save.playerOccurrence;
      playerNameCounter++;
    }
    else
      // move all the other save data to another temp file, so it'd be possible to prepend the data about unique player names to original file
      fputs(save.oneLine, saveFileTemp);
  }

  fclose(saveFile);
  fclose(saveFileTemp);
  fclose(fopen("result.txt", "w")); // remove contents of result.txt file
  saveFile = fopen("result.txt", "a");
  saveFileTemp = fopen("temp.txt", "r");
  unsigned short newPlayersCounter = 0, newPlayersEncountered = 0;
  bool firstPlayerEncountered = false, secondPlayerEncountered = false;
  char typeOfNewPlayer = (saveFileIsNotEmpty) ? 0 : 'd';

  // iterate through the savesForParticularPlayer (gameSave struct) array to check, whether new player names equal to old ones written to the database
  for(unsigned long i = 0; i < uniquePlayerNames; i++){
    if(!firstPlayerEncountered && !strcmp(save.firstPlayerName, savesForParticularPlayer[i].playerSaveName)){
      savesForParticularPlayer[i].playerOccurrence++;
      firstPlayerEncountered = true;
    }
    else if(!firstPlayerEncountered && i == uniquePlayerNames - 1){
      typeOfNewPlayer = (newPlayersEncountered++ == 0) ? 'f' : 'd';
      firstPlayerEncountered = true;
    }
    if(!secondPlayerEncountered && !strcmp(save.secondPlayerName, savesForParticularPlayer[i].playerSaveName)){
      savesForParticularPlayer[i].playerOccurrence++;
      secondPlayerEncountered = true;
    }
    else if (!secondPlayerEncountered && i == uniquePlayerNames - 1){
      typeOfNewPlayer = (newPlayersEncountered++ == 0) ? 's' : 'd';
      secondPlayerEncountered = true;
    }
    fprintf(saveFile, "0\t\t%lu\t\t%s\t\t0\t\t0\n", savesForParticularPlayer[i].playerOccurrence, savesForParticularPlayer[i].playerSaveName);
  }

  // check if the player names are new. 'f': the first player name is new. 's': 2nd one. 'd': both of them.
  switch(typeOfNewPlayer){
    case 'f':
    fprintf(saveFile, "0\t\t%lu\t\t%s\t\t0\t\t0\n", 1, save.firstPlayerName);
    break;
    case 's':
    fprintf(saveFile, "0\t\t%lu\t\t%s\t\t0\t\t0\n", 1, save.secondPlayerName);
    break;
    case 'd':
    fprintf(saveFile, "0\t\t%lu\t\t%s\t\t0\t\t0\n", 1, save.firstPlayerName);
    fprintf(saveFile, "0\t\t%lu\t\t%s\t\t0\t\t0\n", 1, save.secondPlayerName);
    break;
  }

  // write contents back from temp file to original
  gameSave backToResult;
  while(fgets(backToResult.oneLine, sizeof backToResult.oneLine, saveFileTemp) != NULL){
    fputs(backToResult.oneLine, saveFile);
  }

  fclose(saveFileTemp);
  remove("temp.txt");
  free(savesForParticularPlayer);

  // write the new save data
  fprintf(saveFile, "%x\t\t%u\t\t%s\t\t%s\t\t%.42s\n", save.gameID, save.emptyFields, save.firstPlayerName, save.secondPlayerName, gameBoard.board);
  fclose(saveFile);
  printf("\nSaved the game successfully with ID %x.\n", save.gameID);
}

void loadSavedGameMenu() {
    printf("\n1. List all saved games.\n");
    printf("2. List all saved games for a particular player.\n");
    printf("3. Show the board of one of the saved games.\n");
    printf("4. Load a game.\n");
    printf("5. Return to main menu.\n");
    int loadSavedGameNum;
    do {
      scanf("%d", &loadSavedGameNum);
      fflush(stdin);
      notACorrectOption = false;
      switch(loadSavedGameNum){
        case 1:
        listSavedGames();
        break;
        case 2:
        listSavedGamesForPlayer();
        break;
        case 3:
        showBoardForSavedGame();
        break;
        case 4:
        loadSavedGame();
        break;
        case 5:
        mainMenu();
        break;
        default:
        notACorrectOption = true;
        printf("Not a correct option.\n");
        break;
      }
    } while(notACorrectOption);
};

void listSavedGames(){
  gameSave saveOfListSavedGames;

  FILE *saveFile;
  saveFile = fopen("result.txt", "r");
  checkSaveFile(saveFile);

  unsigned long saveFileCounter = 0;
  while(fgets(saveOfListSavedGames.oneLine, sizeof saveOfListSavedGames.oneLine, saveFile) != NULL){
    if(saveOfListSavedGames.oneLine[0] == '0') continue;
    sscanf(saveOfListSavedGames.oneLine, "%x\t\t%u\t\t%[^\t]\t\t%[^\t]\t\t%*s", &saveOfListSavedGames.gameID, &saveOfListSavedGames.emptyFields, saveOfListSavedGames.firstPlayerName, saveOfListSavedGames.secondPlayerName, saveOfListSavedGames.oneDimensionalBoard);
    printf("%lu. Game ID: %x, first player: %s (as X), second player: %s (as O), free fields left: %u\n", ++saveFileCounter, saveOfListSavedGames.gameID, saveOfListSavedGames.firstPlayerName, saveOfListSavedGames.secondPlayerName, saveOfListSavedGames.emptyFields);
  }

  printf("\n");
  loadSavedGameMenu();
  fclose(saveFile);
}

void listSavedGamesForPlayer(){
  FILE *saveForParticularPlayer;
  saveForParticularPlayer = fopen("result.txt", "r");

  checkSaveFile(saveForParticularPlayer);

  printf("\nType in the player name to display all the saves for.\n");
  gameSave listGamesForPlayer;
  do {
    notACorrectOption = false;
    strcpy(listGamesForPlayer.playerSaveName, inputString(stdin, maxPlayerNameLength - 1));
    fflush(stdin);
    if(strlen(listGamesForPlayer.playerSaveName) > maxPlayerNameLength - 1){
      printf("\nPlayer names can't be longer than %u characters.\n\n", maxPlayerNameLength - 1);
      notACorrectOption = true;
    }
  } while(notACorrectOption);

  char _unused, foundName[maxPlayerNameLength];
  unsigned short printPlayerNameInfo = 0;
  bool nameFound = false;
  gameSave printSaveData;
  unsigned long saveFileCounter = 0;

  while(fgets(listGamesForPlayer.oneLine, sizeof listGamesForPlayer.oneLine, saveForParticularPlayer) != NULL){
    if(listGamesForPlayer.oneLine[0] == '0'){
      sscanf(listGamesForPlayer.oneLine, "%u\t\t%lu\t\t%[^\t]\t\t%u\t\t%*u", &_unused, &listGamesForPlayer.playerOccurrence, listGamesForPlayer.firstPlayerName, &_unused, &_unused);
      if(!nameFound && !strcmp(listGamesForPlayer.firstPlayerName, listGamesForPlayer.playerSaveName)){
        strcpy(foundName, listGamesForPlayer.playerSaveName);
        nameFound = true;
      }
    }
    else {
      if(!nameFound){
        printf("\nThe name %s wasn't found in the database. Returning to the menu.\n", listGamesForPlayer.playerSaveName);
        loadSavedGameMenu();
        return;
      }
      if(!printPlayerNameInfo++) printf("\nSave files for player %s:\n", foundName);
      sscanf(listGamesForPlayer.oneLine, "%x\t\t%u\t\t%[^\t]\t\t%[^\t]\t\t%*s", &printSaveData.gameID, &printSaveData.emptyFields, printSaveData.firstPlayerName, printSaveData.secondPlayerName, printSaveData.oneDimensionalBoard);
      if(!strcmp(foundName, printSaveData.firstPlayerName) || !strcmp(foundName, printSaveData.secondPlayerName))
        printf("%lu. Game ID: %x, first player: %s (as X), second player: %s (as O), free fields left: %u\n", ++saveFileCounter, printSaveData.gameID, printSaveData.firstPlayerName, printSaveData.secondPlayerName, printSaveData.emptyFields);
    }
  }

  loadSavedGameMenu();
}

void showBoardForSavedGame(){
    FILE *saveFileBoard;
    saveFileBoard = fopen("result.txt", "r");

    checkSaveFile(saveFileBoard);


  printf("\nEnter an ID of one of the saved games to print its board.\n");
  unsigned long gameIDBoard;
  scanf("%x", &gameIDBoard);
  fflush(stdin);

  gameSave saveBoard;
  bool foundSave = false;
  char matrixSaveFile[matrixHeight][matrixWidth];
  // copy the board, so the global variable doesnt change
  for(unsigned short i = 0; i < matrixHeight; i++){
    for(unsigned short j = 0; j < matrixWidth; j++){
        matrixSaveFile[i][j] = matrixGlobal[i][j];
    }
  }

  while(fscanf(saveFileBoard, "%x\t\t%u\t\t%[^\t]\t\t%[^\t]\t\t%s", &saveBoard.gameID, &saveBoard.emptyFields, saveBoard.firstPlayerName, saveBoard.secondPlayerName, saveBoard.board) != EOF){
    if(saveBoard.gameID != 0 && saveBoard.gameID == gameIDBoard) {
      foundSave = true;
      printf("\nGame board for the game %x:\n\n", gameIDBoard);
      for (unsigned short i = 0; i < matrixHeight; i++) {
          for(unsigned short j = 0; j < matrixWidth; j++){
            if(i >= 1 && j >= 6 && j <= matrixWidth - 2 && !(j % 2)){
              matrixSaveFile[i][j] = saveBoard.board[i - 1][(j / 2) - 3];
            } // add the Xs and Os to the board and print it
            printf("%c", matrixSaveFile[i][j]);
          }
      }
      loadSavedGameMenu();
      break;
    }
  }

  if(!foundSave) {
    printf("\nGame with ID %x (hex) wasn't found.\n", gameIDBoard);
    loadSavedGameMenu();
  }

}

void loadSavedGame(){
  FILE *saveFileLoad;
  saveFileLoad = fopen("result.txt", "r");

  checkSaveFile(saveFileLoad);

  printf("\nEnter the ID of the game to be loaded.\n");
  unsigned long gameIDLoad;
  scanf("%x", &gameIDLoad);
  fflush(stdin);

  gameSave saveLoad;
  bool foundSave = false;

  while(fscanf(saveFileLoad, "%x\t\t%u\t\t%[^\t]\t\t%[^\t]\t\t%s", &saveLoad.gameID, &saveLoad.emptyFields, saveLoad.firstPlayerName, saveLoad.secondPlayerName, saveLoad.oneDimensionalBoard) != EOF){
    if(saveLoad.gameID != 0 && saveLoad.gameID == gameIDLoad){
      startAGame(saveLoad.firstPlayerName, saveLoad.secondPlayerName, saveLoad.oneDimensionalBoard);
      foundSave = true;
      break;
    }
  }

  if(!foundSave) {
    printf("\nGame with ID %x (hex) wasn't found.\n", gameIDLoad);
    loadSavedGameMenu();
  }
}

int maxBetween2Ints(int num1, int num2)
{
    return (num1 > num2) ? num1 : num2;
}

// function for unlimited string input in console/any other file without buffer overflow
char *inputString(FILE* fp, size_t size){ // The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0;
    str = realloc(NULL, sizeof(*str)*size); // size is start size
    if(!str)return str;
    while(EOF!=(ch=fgetc(fp)) && ch != '\n'){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(*str)*(size+=16));
            if(!str)return str;
        }
    }
    str[len++]='\0';

    return realloc(str, sizeof(*str)*len);
}
