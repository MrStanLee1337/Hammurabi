
#include <iostream>
#include <fstream>
#include "json.hpp"
#include <random>

template <typename T>
void print(const T& s) { std::cout << s; }
template <typename T>
void println(const T& s) { std::cout << s << "\n"; }


template <typename T>
T myClamp(const T low, const T high, const T t) {
    return 
    t < low ? low : 
    t > high ? high :
    t;
}

#define debug(x) std::cout << __LINE__ << " | " << #x << " | " << x << std::endl;

using json = nlohmann::json;
using myType = float;

//g_ is for global vars
const std::string g_saveFile = "savefile.json";
json g_data;//data of game resources
bool g_starvationGameOver = false;
auto rnd = std::mt19937(time(NULL));


void saveDataToJSON();
void getDataFromJSON();
void createNewSave();

void createMyLordText();
void mainMenu();
void gameMenu();
void createNewSave();

//print game text
void myLordText();

//parse string and convert %keys% to values from JSON
void parseAndConvertString(std::string& str);

//game logic
void playerInput();
void acresToChange();
void grainToUse();
void nextRoundMove();//rules for moving to the next round
bool isStarvationGameOver();
void calcStarvationPerYear(int round);//calc starvation percent for round + 1
void Congratz();//congratulations on finishing the game

void gameMenu();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(){
    setlocale(LC_ALL, "ru");
    createMyLordText();
    mainMenu();
    gameMenu();
    system("pause");
    return 0;
}


void saveDataToJSON() {
    std::ofstream file(g_saveFile);
    if (!file.is_open()) return;
    file << g_data.dump(4);//4 is offset
    file.close();
}

void getDataFromJSON() {
    std::ifstream file(g_saveFile);
    if (!file.is_open()) return;
    try {
        g_data = json::parse(file);
    }
    catch (...) {
        return;
    }
}

void createNewSave() {
    g_data.clear();
    g_data["people"] = 100;
    g_data["grain"] = 2800;
    g_data["grainGot"] = 0;
    g_data["grainGotPerAcre"] = 0;
    g_data["grainToEat"] = 0;
    g_data["acre"] = 1000;
    g_data["acreToSeed"] = 0;
    g_data["round"] = 1;
    g_data["starvation"] = 0;//starved people
    g_data["newcomer"] = 0;
    g_data["plague"] = false;
    g_data["acrePrice"] = []() {return rnd() % 10 + 17;}();
    g_data["miceEaten"] = 0;
    g_data["round"] = 1;
    g_data["maxRound"] = 10;
    g_data["starvationThisYear"] = 0;// from 0. to 1.
    g_data["starvationPerYear"] = 0;// from 0. to 1.
    saveDataToJSON();
}

void mainMenu() {
    println("Игра Hammurabi!");
    getDataFromJSON();

    bool isNewSave = false;
    if (g_data.contains("round")) {
        isNewSave = g_data["round"] == 1;
    }
    else {
        isNewSave = true;
    }

    if (isNewSave) {
        createNewSave();
        saveDataToJSON();
    }
    else {
        while (true) {
            println("Загрузить игру(1) или начать заного(0)?");
            int save = 0; std::cin >> save;
            debug(save);
            if (save == 0) {
                createNewSave();
                saveDataToJSON();
                break;
            }
            else if (save == 1) {
                //getDataFromJSON(); done already
                break;
            }
            else {
                println("Некорректный ввод.");
            }
        }
    }

}

void createMyLordText() {
    std::ofstream file("MyLordText.txt");
    file << R"(
Мой повелитель, соизволь поведать тебе
в году %round% твоего высочайшего правления:
%starvation% человек умерли с голоду;
%newcomer% человек прибыли в наш великий город;
%plague% Чума уничтожила половину населения;
Население города сейчас составляет %people% человек;
Мы собрали %grainGot% бушелей пшеницы, по %grainGotPerAcre% бушеля с акра;
Крысы истребили %miceEaten% бушелей пшеницы, оставив %grain% бушеля в амбарах;
Город сейчас занимает %acre% акров;
1 акр земли стоит сейчас %acrePrice% бушелей;
)";
    file.close();
}

void gameMenu() {
    while (true) {
        getDataFromJSON();
        myType round = g_data["round"];
        if (round >= static_cast<myType>(g_data["maxRound"])) break;
        myLordText();
        playerInput();
        nextRoundMove();
        if (isStarvationGameOver()) return;
        calcStarvationPerYear(round);
        g_data["round"] = round + 1;
        saveDataToJSON();
        println("Игра сохранена.\n");
    }
    Congratz();
}



void myLordText() {//first act of round
    std::ifstream file("MyLordText.txt");
    if (!file.is_open()) return;
    std::string str;

    println("");
    while (std::getline(file, str)) {
        parseAndConvertString(str);
        if(str != "") println(str);
    }
    println("");

    file.close();
}

void parseAndConvertString(std::string& str) {
    //parse string
    std::vector<std::pair<myType, std::string>> replaces;//int index start, string value
    std::string replaceStr = "";
    myType indexStart = -1;
    bool f = false;//flag to copy symbols
    for (myType i = 0; i < size(str); i++) {

        if (str[i] == '%') {
            if (f) {
                replaces.emplace_back(make_pair(indexStart, replaceStr));
                replaceStr = "";
            }
            else {
                indexStart = i;
            }
            f = !f;
        }
        else {
            if (f) replaceStr += str[i];
        }
    }
    
    //convert string
    for (auto Pair = replaces.rbegin(); Pair != replaces.rend(); Pair++) {
        std::string KeyForJSON = (*Pair).second;
       
        
        if (KeyForJSON == "starvation" && !g_data.value(KeyForJSON, 0)) {//if none or zero then not to print
            str = "";
            return;
        }
        if (KeyForJSON == "plague" && !g_data.value(KeyForJSON, false)) {
            str = "";
            return;
        }
        if (KeyForJSON == "newcomer" && !g_data.value(KeyForJSON, 0)) {
            str = "";
            return;
        }
        if (KeyForJSON == "grainGot" && !g_data.value(KeyForJSON, 0)) {
            str = "";
            return;
        }

        if (g_data.contains(KeyForJSON)) {
            int ValueFromJSON = g_data[KeyForJSON];
            str.replace((*Pair).first, size((*Pair).second) + 2, std::to_string(ValueFromJSON));
        }
    }

}

void playerInput() {
    acresToChange();
    grainToUse();
}

void acresToChange() {
    myType acresPrice = g_data["acrePrice"];
    myType grain = g_data["grain"];
    myType acres = g_data["acre"];
    while (true) {//buy
        println("Введите количество акров, которое вы хотите купить.");
        int acresCountToBuy = 0; std::cin >> acresCountToBuy;
        debug(acresCountToBuy);
        if (acresCountToBuy * acresPrice > grain) println("У вас недостаточно пшеницы.");
        else if (acresCountToBuy < 0) println("Некорректный ввод.");
        else {
            acres += acresCountToBuy;
            grain -= acresCountToBuy * acresPrice;
            break;
        };
    }

    while (true) {//sell
        println("Введите количество акров, которое вы хотите продать.");
        int acresCountToSell = 0; std::cin >> acresCountToSell;
        if (acresCountToSell < 0) println("Некорректный ввод");
        else if (acresCountToSell > acres) println("У вас недостаточно акров.");
        else {
            acres -= acresCountToSell;
            grain += acresCountToSell * acresPrice;
            break;
        }
    }
    g_data["grain"] = grain;
    g_data["acre"] = acres;

}

void grainToUse() {
    myType grain = g_data["grain"];
    myType acres = g_data["acre"];
    myType people = g_data["people"];
    while (true) {
        println("Введите количество бушелей пшеницы в качестве еды для жителей.");
        int grainToEat = 0; std::cin >> grainToEat;
        if (grainToEat < 0) println("Некорректный ввод");
        else if (grainToEat > grain) println("У вас недостаточно пшеницы.");
        else {
            g_data["grainToEat"] = grainToEat;
            break;
        }
    }

    while (true) {
        println("Введите количество акров земли, которые необходимо засеять.");
        myType acresToSeed = 0; std::cin >> acresToSeed;
        if (acresToSeed < 0) println("Некорректный ввод");
        else if (acresToSeed > acres) println("У вас недостаточно акров.");
        else {
            acresToSeed = std::min(acresToSeed, people * 10);
            acresToSeed = std::min(acresToSeed, grain * 2);
            g_data["acresToSeed"] = acresToSeed;

            break;
        }
    }
}

void nextRoundMove() {

    // 1 step of algorithm
    int grainPerAcre = []() {
        return rnd() % 6 + 1;
    }();

    // 2(implicit)
    myType grain = g_data["grain"];
    myType acresToSeed = g_data["acresToSeed"];
    grain -= acresToSeed/2;
    myType grainGot = grainPerAcre * g_data["acresToSeed"];
    grain += grainGot;

    // 2 
    myType miceEaten = [grain]() {
        return grain * (rnd() % 8) / 100;
        } ();
    grain -= miceEaten;

    // 3
    myType people = g_data["people"];
    myType grainToEat = g_data["grainToEat"];
    myType starvation = [people, grainToEat]() {
        return std::max(myType(0), people - grainToEat / 20);
        } ();
    float starvationThisYear = static_cast<float> (starvation) / people;
    people -= starvation;
    grain -= people * 20;

    // 4
    if (starvationThisYear > 0.45) {//if starvation > 45% of people
        g_starvationGameOver = true;
        g_data["starvation"] = starvation;
        g_data["starvationThisYear"] = starvationThisYear;
        g_data["people"] = people;
        return;
    }

    // 5
    myType newcomer = [starvation, grainPerAcre, grain]() {
        return myClamp(0.f, 50.f, starvation / 2 + (5 - grainPerAcre) * grain / 600 + 1);
        } ();
    people += newcomer;

    // 6
    [&people]() {
        if (rnd() % 100 < 15) {
            g_data["plague"] = true;
            people /= 2;
        }
        else {
            g_data["plague"] = false;
        }
        }();

    // 7 save changes
    g_data["grainGotPerAcre"] = grainPerAcre;
    g_data["grain"] = grain;
    g_data["grainGot"] = grainGot;
    g_data["people"] = people;
    g_data["miceEaten"] = miceEaten;
    g_data["starvation"] = starvation;
    g_data["starvationThisYear"] = starvationThisYear;
    g_data["newcomer"] = newcomer;

}

bool isStarvationGameOver() {
    if (g_starvationGameOver) {
        myType starvation = g_data["starvation"];
        float starvationThisYear = g_data["starvationThisYear"];
        myType percent = static_cast<myType>(starvationThisYear * 100);
        println("От голода умерло " + std::to_string(starvation) + " населения, что составляет " + std::to_string(percent) + "%. Ваше правление окончено.");
        return true;
    }
    return false;
}

void calcStarvationPerYear(int round) {
    float starvationPerYear = g_data["starvationPerYear"];//past years
    float starvationThisYear = g_data["starvation"];
    starvationPerYear = ((round - 1) * starvationPerYear + starvationThisYear) / round;
    g_data["starvationPerYear"] = starvationPerYear;
}

void Congratz() {
    myType maxRound = g_data["maxRound"];
    println("Поздравляю! Вы продержались " + std::to_string(maxRound) + "лет! Время подвести итоги:");
    myType acres = g_data["acre"];
    myType people = g_data["people"];
    float P = g_data["starvationPerYear"];
    float L = (static_cast<float> (acres) / people);
    if (P > 0.33 and L < 7) println("Из-за вашей некомпетентности в управлении, народ устроил бунт и вас выгнали.");
    else if (P > 0.1 and L < 9) println("Вы правили железной рукой, подобно Ивану Грозному. Вас не желают видеть правителем.");
    else if (P > 0.3 and L < 10) println("Вы неплохо справились. У вас остались недоброжелатели, но многи хотели бы видеть вас правителем.");
    else println("Фантастика, вы превосходно справились со своими обязанностями!");
}