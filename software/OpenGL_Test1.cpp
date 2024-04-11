#include <GL/glut.h>
#include <iostream>
#include <windows.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>

using namespace std;

HANDLE SerialCOM, SerialEvent;   //Дескриптор COM-порта

int winSizeX = 180 * 3, winSizeY = 180 * 3;
int matrixSizeX = 18, matrixSizeY = 18;
int quadsSideX = 0, quadsSideY = 0;
int quadsQuantity = 0;
float colorStep = 0.0;

bool screenUPDT = 0;

uint8_t colorBuff[1000] = { 0, };


class Serial_c {
public:

    struct serial_t {
        uint32_t buff_p = 0;
        uint8_t buff[1000] = { 0, };
        struct flags_t {
            bool start = FALSE;   // устанавливается при приёме первого байта
            bool end = FALSE;     // устанавливается после таймаута
        } flags;
        struct timeout_t {
            uint32_t counter = 0;
            uint32_t value = 1;
        } timeout;
    };

    serial_t recive;    // буфер приёмника
    serial_t transmit;  // буфер передатчика

    bool СheckTimeout() {
        if (recive.flags.start == FALSE) return FALSE;
        if (recive.timeout.counter > recive.timeout.value) {
            recive.flags.start = FALSE;
            recive.flags.end = TRUE;
            return TRUE;
        }
        recive.timeout.counter++;
        return FALSE;
    }

    //**************** приём данных ********************************

    void StartTimeout() {
        recive.timeout.counter = 0;
    }

    bool Available() {
        return recive.flags.end;
    }

    void Read(uint8_t* buff) {   // чтение данных в буффер
        uint32_t data_p = 0;
        while (data_p < recive.buff_p) {
            *buff = recive.buff[data_p];  // копируем входной буфер в новый массив
            recive.buff[data_p] = 0;      // очищаем входной буфер
            buff++;
            data_p++;
        }
        recive.flags.end = FALSE;
    }

    void Recive() {
        // читаем данные из COM-порта
        DWORD  iSize = 0;
        uint8_t data = 0;
        ReadFile(SerialCOM, &data, 1, &iSize, 0);  // получаем 1 байт

        if (iSize > 0) {
            //cout << hex << uppercase << (int)data;
            //cout << " ";
            if (recive.flags.start == FALSE) {    // если приняли первый байт последовательности
                recive.buff_p = 0;
                recive.flags.start = TRUE;
                recive.flags.end = FALSE;

                
            }

            recive.buff[recive.buff_p] = data;
            recive.buff_p++;
            StartTimeout();
        }
    }

    //**************** передача данных ********************************
    /*
    void Write(uint8_t* buff) {

    }
    */

    void Write(uint8_t* buff, uint8_t length) {
        if (transmit.flags.start == TRUE) return;
        transmit.buff_p = 0;
        while (transmit.buff_p < length) {
            transmit.buff[transmit.buff_p] = *buff;
            buff++;
            transmit.buff_p++;
        }
        transmit.flags.start = TRUE;
    }

    void Transmit() {
        if (transmit.flags.start == TRUE) {
            DWORD feedback;
            WriteFile(SerialCOM, &transmit.buff[0], transmit.buff_p, &feedback, 0);
            transmit.flags.start = FALSE;
            transmit.flags.end = TRUE;
        }
    }
};

Serial_c Serial;

void comReciveThread() {
    // создаём COM-порт
    LPCTSTR portName = L"COM6";
    SerialCOM = CreateFile(portName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (SerialCOM == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, TEXT("Невозможно открыть последовательный порт"), TEXT("Error"), MB_OK);
    }

    // настраиваем COM-порт
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(SerialCOM, &dcbSerialParams))
    {
        cout << "getting state error\n";
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(SerialCOM, &dcbSerialParams))
    {
        cout << "error setting serial port state\n";
    }

    while (1) {
        Serial.Recive();
    }
}

void comTransmitThread() {
    while (1) {
        Serial.Transmit();
    }
}

void timerThread() {
    while (1) {
        // проверяем таймаут приёма COM
        Serial.СheckTimeout();
        Sleep(1);
    }
}



void myinit(void)
{
    glClearColor(0,0,0,0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, winSizeX, 0.0, winSizeY);

    // создаём квадратную матрицу 18*18
    cout << "matrix param:" << endl;

    quadsSideX = winSizeX / matrixSizeX;
    quadsSideY = winSizeY / matrixSizeY;

    cout << "quads Side X=" << quadsSideX << endl;
    cout << "quads Side Y=" << quadsSideY << endl;

    quadsQuantity = matrixSizeX * matrixSizeY;

    cout << "quads Quantity=" << quadsQuantity << endl;

    colorStep = 1.0 / (float)quadsQuantity;

    cout << "color Step=" << colorStep << endl;
}

void setPixel(GLint x, GLint y)
{
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void display() {
    // запускаем цикл отрисовки
    //while(1) {
        //if (screenUPDT == 0) continue;
        glClear(GL_COLOR_BUFFER_BIT);

        // цвет задаём в оттенках серого 
        float color = 0.1;
        int elementCounter = 0; // позиция элемента в массиве принятых байт

        // формируем матрицу
        for (int x = 0; x < matrixSizeX; x++) {
            for (int y = 0; y < matrixSizeY; y++) {
                color = (float)colorBuff[elementCounter]/40;
                glColor3f(color, color, color);
                glBegin(GL_QUADS);
                glVertex2f(x * quadsSideX, y * quadsSideX);
                glVertex2f(x * quadsSideX + quadsSideX, y * quadsSideX);
                glVertex2f(x * quadsSideX + quadsSideX, y * quadsSideX + quadsSideY);
                glVertex2f(x * quadsSideX, y * quadsSideX + quadsSideY);
                glEnd();

                //cout << "element: " << elementCounter << " color: " << color << endl;
                elementCounter++;
            }
        }

        glFlush();
        glutSwapBuffers();

        screenUPDT = 0;
    //}
}



void mainThread() {
    while (1) {
        
        if (Serial.Available() == TRUE) {
            Serial.Read(&colorBuff[0]);

            
            // выводим полученные значения
            int byteCounter = 0;
            cout << "serial data: " << endl;
            for (int x = 0; x < matrixSizeX; x++) {
                for (int y = 0; y < matrixSizeY; y++) {
                    cout << dec << uppercase << (int)colorBuff[byteCounter];
                    cout << " ";
                    byteCounter++;
                }
                cout << endl;
            }
            
            screenUPDT = 1;
            
        }
       
        Sleep(10);
    }
}

void timf(int value) // Timer function 
{ 
    glutPostRedisplay(); // Redraw windows 
    glutTimerFunc(20, timf, 0); // Setup next timer 
}


int main(int argc, char** argv) {
    std::thread tA(comReciveThread);        // работа с COM портом (асинхронный приём данных)
    std::thread tD(comTransmitThread);      // работа с COM портом (асинхронная передача данных)
    std::thread tB(mainThread);             // основной поток
    std::thread tC(timerThread);            // системный таймер

    glutInit(&argc, argv);
    glutInitWindowSize(winSizeX, winSizeY);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Mouse camera viewer");
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    myinit();
    glutTimerFunc(20, timf, 0); // задержка перед обновлением картинки на экране

    glutDisplayFunc(display);
    glutMainLoop();

    tA.join();
    tB.join();
    tC.join();
    tD.join();

    return 0;
}