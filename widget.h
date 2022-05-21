#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtCharts/QChart>
//基础窗口

//串口所需
#include <QtSerialPort>
#include<QSerialPortInfo>
#include"qcustomplot.h"
using namespace QtCharts;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void first_init();
    void my_connect();
    //示波器
    void build_chart();
    void update_paint(QByteArray info);
    void plotCustom(QByteArray info);
    void receiveInfo();
    void on_openOscill_clicked();//
    void update_paint2(QByteArray info);

    void buildChart();                                      //创建图表函数
    //开始画图
    void start_paint();

    //槽函数
    void cheak_serial();
    void set_serial();
    void get_info();
    void Is_PopoutSightless();
    void check();
    void test();
    void my_connect2();
    void update_paint3(QByteArray info);

private:
    Ui::Widget *ui;
    QSerialPort port;

    double num_point;
    QVector<QString> name_line;
    QVector<double> XData;
    QVector <QVector<double>>   XData2;
    QVector <QVector<double>>   YData;

    QStringList Key_of_Json;
    QStringList Customplot_names;

    QString frontData;
    QString backData;
    QString oneData;
    double lon;
    double rebomm;
    int black;
    int my_black;
    int test_point;

    QVector<double>::iterator start;//迭代器

    bool flag_error;
    bool serial_ready;
    bool oscilloscope_ready;
    bool Is_Popout;



};

#endif // WIDGET_H
