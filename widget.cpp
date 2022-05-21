 #include "widget.h"
#include "ui_widget.h"
#include<QPushButton>
#include<QDebug>
#include<QVector>
#include<QLabel>
#include "qcustomplot.h"

const int color[10]={
    0xff0000,0x00ff00,0x0000ff,0xff00ff,
    0xff8000,0xc0ff00,0x00ffff,0x8000ff,0x000001,
};

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);


    first_init();
    my_connect();

}

Widget::~Widget()
{
    delete ui;
}


//信号链接函数
void Widget::my_connect()
{
    connect(ui->btn_cheak,&QPushButton::clicked,this,&Widget::cheak_serial);
    connect(ui->btn_start_end,&QPushButton::clicked,this,&Widget::set_serial);
    connect(&port,&QSerialPort::readyRead,this,&Widget::get_info);
    //connect(ui->btn_openocc_2,&QPushButton::clicked,this,&Widget::test);
    //connect(&port,&QSerialPort::readyRead,this,&Widget::check);
    connect(ui->btn_openocc,&QPushButton::clicked,this,&Widget::build_chart);
}

void Widget::my_connect2()
{
    connect(ui->btn_cheak,&QPushButton::clicked,this,&Widget::cheak_serial);
    connect(ui->btn_start_end,&QPushButton::clicked,this,&Widget::set_serial);
    connect(&port,&QSerialPort::readyRead,this,&Widget::receiveInfo);
    //connect(ui->btn_openocc_2,&QPushButton::clicked,this,&Widget::test);
    //connect(&port,&QSerialPort::readyRead,this,&Widget::check);
    connect(ui->btn_openocc,&QPushButton::clicked,this,&Widget::on_openOscill_clicked);
}

void Widget::check()
{
    QByteArray info;
    info = port.readAll();


    qDebug()<<info ;
}

void Widget::on_openOscill_clicked()//
{
    /*--------------------------------
     *  打开示波器
     *-------------------------------*/
    if (!oscilloscope_ready){
        if (port.isOpen()){
            oscilloscope_ready = true  ;
            buildChart();
            ui->btn_openocc->setText("关闭示波器");
        }else{
            QMessageBox box;
            box.setWindowTitle(tr("警告"));
            box.setWindowIcon(QIcon(":/Images/Icons/Alien 2.png"));
            box.setIcon(QMessageBox::Warning);
            box.setText(tr(" 未检测到串口，请确认串口是否打开？"));
            box.exec();
        }
    }else{
        // 置空所有状态值
        oscilloscope_ready = false;
        this->num_point = 0;
        this->name_line.clear();
        this->XData.clear();
        this->YData.clear();
        this->Customplot_names.clear();
        // 清楚画布
        ui->customplot->clearGraphs();
        ui->customplot->legend->setVisible(false);
        ui->customplot->replot();
        ui->customplot->setBackground(QBrush(QColor("#474848")));
        ui->btn_openocc->setText("打开示波器");
    }
}


void Widget::first_init()
{
    lon = 100;
    rebomm = 0;//这是重载的时候的计算法
    serial_ready = false;
    oscilloscope_ready = false;
    num_point = 0;
    black = 0;//作为间隔用的
    my_black = 5;//作为预期的间隔

    //外观定性
    ui->customplot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom|QCP::iSelectPlottables);
    ui->customplot->setBackground(QBrush(QColor("#474848")));
    ui->customplot->yAxis2->setVisible(true);//显示y轴2
    ui->customplot->xAxis2->setVisible(true);//显示x轴2
    ui->customplot->axisRect()->setupFullAxesBox();
    ui->customplot->yAxis->setRange(0,3.3);
    ui->customplot->yAxis2->setRange(0,3.3);

    ui->customplot->xAxis->setRange(0,lon);
    ui->customplot->xAxis2->setRange(0,lon);

}
//检查可用端口并更新
void Widget::cheak_serial()
{
    if(QObject::sender() == ui->btn_cheak)
    {
        ui->com_com->clear();
        //接收
        QList<QSerialPortInfo> avail = QSerialPortInfo::availablePorts();
        foreach (const QSerialPortInfo &s , avail) {
            ui->com_com->addItem(s.portName());
        }
    }

}

void Widget::update_paint(QByteArray info)
{
    if(oscilloscope_ready)
    {
        QStringList DataKeys;
        QJsonParseError JasonError;
        QJsonDocument JsonDoc;
        JsonDoc = QJsonDocument::fromJson(info,&JasonError);

        //核查信息的准确性，很重要
        if(JasonError.error != QJsonParseError::NoError)
        {
            ui->listWidget->addItem("json转换出错");
            return;
        }

        QJsonObject JsonObject = JsonDoc.object();
        DataKeys = JsonObject.keys();



        //设置标签——将json和customplot连接起来——如果下位机多了一个输出，也能够立刻转变
        if(Customplot_names != DataKeys){
            Customplot_names = DataKeys;
            ui->listWidget->addItem("update keys");

            //让customplot跟通道连接起来
            name_line.clear();
            XData.clear();
            YData.clear();

            //把以前的擦掉
            ui->customplot->clearGraphs();
            ui->customplot->legend->setVisible(false);
            ui->customplot->replot();

            ui->customplot->legend->setTextColor(Qt::black);
            ui->customplot->legend->setBrush(QColor(0,0,0,0));//啥颜色都没有
            ui->customplot->legend->setVisible(true);

            //设置线颜色+组合信息
            for(int i =0;i<Customplot_names.size();i++)
            {
                ui->customplot->addGraph();//增线
                ui->customplot->graph(i)->setPen(QPen(color[i]));
                ui->customplot->graph(i)->setName(Customplot_names.at(i));
            }
        }


        //处理xdata和ydata///////////////////////////////////////////////////////////////
        //再塞进图里


            //*****************************************************************************
            //10当作0处理
            if(XData.size() < lon)//当跃出边界的时候_如果是小于，则刚好会载100停下
                XData.push_back(num_point);//压入x坐标
            //超出屏幕外的，仅改y轴

            //*****************************************************************************
            YData.resize(Customplot_names.size());//根据需要创建线的数量
            double tmp;//桶，用来装还原出来的数据
            //QString item_tmp;
        //**********************************************************************//
//                                            //轴跟线的函数
//            if(num_point>lon)//当跃出边界时候
//            {
//                if((int)num_point % (int)lon == 0)//每次跃出的时候
//                    ui->customplot->xAxis->setRange(num_point,num_point+lon);//跟屏
//            }
//            else
//                ui->customplot->xAxis->setRange(0,lon);//第一屏
        //**********************************************************************//

            //test


            //根据两根线的要求
            for(int i = 0; i < Customplot_names.size();i++)
            {
                //根据名字调出数据，转成double
                tmp = JsonObject.value(Customplot_names[i]).toDouble();

                //**************************************************************************
                //第一屏，没满的时候
                qDebug()<<XData.size();
                qDebug()<<YData[i].size();
                if(YData[i].size() == lon)
                {
                    //此处是第二屏，由10，相当于0开始
                    qDebug()<<YData[i].size();
                    qDebug()<<rebomm;

                    YData[i][rebomm] = tmp*3.3/4096;//结果循环存到第一位0去
                    //
                }
                else
                YData[i].push_back(tmp*3.3/4096);

                //**************************************************************************

                //两种不同的给data的方式************************************************************
                //ui->customplot->graph(i)->addData(XData[num_point],YData[i][num_point]);
                //性能不一样，这个是整根线更新，会慢一点
                ui->customplot->graph(i)->addData(XData,YData[i]);
                //*******************************************************************************
                //item_tmp = item_tmp + Customplot_names.at(i)+" : " + QString::number(YData[i][rebomm]) + "V  ";

                //跟屏********

                //***********
            }
            rebomm++;
            num_point++;
            qDebug()<<rebomm;
            if(rebomm == lon)//如果下一个点是10，当作0
            {
                rebomm = 0;//rebomm归零
                for(int i = 0; i < 2;i++)
                {
                    ui->customplot->graph(i)->data().data()->clear();
                }
            }





            //连续调用，需要一个优化
            ui->customplot->replot(QCustomPlot::rpQueuedReplot);//重绘
        }
        else
        {

        }
    }


void Widget::update_paint2(QByteArray info)
{
    if(oscilloscope_ready)
    {
        QStringList DataKeys;
        QJsonParseError JasonError;
        QJsonDocument JsonDoc;
        JsonDoc = QJsonDocument::fromJson(info,&JasonError);

        //核查信息的准确性，很重要
        if(JasonError.error != QJsonParseError::NoError)
        {
            ui->listWidget->addItem("json转换出错");
            return;
        }

        QJsonObject JsonObject = JsonDoc.object();
        DataKeys = JsonObject.keys();



        //设置标签——将json和customplot连接起来——如果下位机多了一个输出，也能够立刻转变
        if(Customplot_names != DataKeys){
            Customplot_names = DataKeys;
            ui->listWidget->addItem("update keys");

            //让customplot跟通道连接起来
            name_line.clear();
            XData.clear();
            YData.clear();

            //把以前的擦掉
            ui->customplot->clearGraphs();
            ui->customplot->legend->setVisible(false);
            ui->customplot->replot();

            ui->customplot->legend->setTextColor(Qt::black);
            ui->customplot->legend->setBrush(QColor(0,0,0,0));//啥颜色都没有
            ui->customplot->legend->setVisible(true);

            //设置线颜色+组合信息
            for(int i =0;i<Customplot_names.size();i++)
            {
                ui->customplot->addGraph();//增线
                ui->customplot->graph(i)->setPen(QPen(color[i]));
                ui->customplot->graph(i)->setName(Customplot_names.at(i));
            }
        }


        //处理xdata和ydata///////////////////////////////////////////////////////////////
        //再塞进图里


            //*****************************************************************************
            //10当作0处理
            //if(XData.size() < lon)//当跃出边界的时候_如果是小于，则刚好会载100停下
                XData.push_back(num_point);//压入x坐标
            //超出屏幕外的，仅改y轴

            //*****************************************************************************
            YData.resize(Customplot_names.size());//根据需要创建线的数量
            double tmp;//桶，用来装还原出来的数据
            QString item_tmp;
        //**********************************************************************//
                                            //轴跟线的函数
            if(num_point>lon)//当跃出边界时候
            {
                if((int)num_point % (int)lon == 0)//每次跃出的时候
                    ui->customplot->xAxis->setRange(num_point,num_point+lon);//跟屏
            }
            else
                ui->customplot->xAxis->setRange(0,lon);//第一屏
        //**********************************************************************//

            //test


            //根据两根线的要求
            for(int i = 0; i < Customplot_names.size();i++)
            {
                //根据名字调出数据，转成double
                tmp = JsonObject.value(Customplot_names[i]).toDouble();

                //**************************************************************************
                //第一屏，没满的时候
                qDebug()<<XData.size();
                qDebug()<<YData[i].size();
//                if(YData[i].size() == lon)
//                {
//                    //此处是第二屏，由10，相当于0开始
//                    qDebug()<<YData[i].size();
//                    qDebug()<<rebomm;

//                    YData[i][rebomm] = tmp*3.3/4096;//结果循环存到第一位0去
//                    //
//                }
//                else
                YData[i].push_back(tmp*3.3/4096);

                //**************************************************************************

                //两种不同的给data的方式************************************************************
                ui->customplot->graph(i)->addData(XData[num_point],YData[i][num_point]);
                //性能不一样，这个是整根线更新，会慢一点
                //ui->customplot->graph(i)->addData(XData,YData[i]);
                //*******************************************************************************
                item_tmp = item_tmp + Customplot_names.at(i)+" : " + QString::number(YData[i][num_point]) + "V  ";

            }
            num_point++;
            qDebug()<<rebomm;





            //连续调用，需要一个优化
            ui->customplot->replot(QCustomPlot::rpQueuedReplot);//重绘
        }
        else
        {

        }
    }
void Widget::plotCustom(QByteArray info)//
{
        //避免中文乱码
        QTextCodec *tc = QTextCodec::codecForName("GBK");//找编码器
        QString tmpQStr = tc->toUnicode(info);//

        // 若示波器打开，开始解析数据
        if (oscilloscope_ready){
            QStringList datakeys;

            QJsonParseError jsonError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(info, &jsonError);

            if (jsonError.error != QJsonParseError::NoError)
            {
                qDebug() << "parse json object failed, " << jsonError.errorString();
                ui->listWidget->addItem("parse json object failed!");
                return;
            }
            QJsonObject jsonObj = jsonDoc.object();

            qDebug() << "parse json object Successfully";
            datakeys = jsonObj.keys();      // 获取通道名称
            qDebug() << datakeys;

            if (Customplot_names != datakeys){
                // 只能够设置一次标签
                qDebug() << "update keys";
                Customplot_names = datakeys;

                this->num_point = 0;
                this->name_line.clear();
                this->XData.clear();
                this->YData.clear();

                // 清除画布
                ui->customplot->clearGraphs();
                ui->customplot->legend->setVisible(false);
                ui->customplot->replot();

                ui->customplot->legend->setVisible(true);  //右上角指示曲线的缩略框
                ui->customplot->legend->setBrush(QColor(100, 100, 100, 0));//设置图例背景颜色，可设置透明

                ui->customplot->legend->setTextColor(Qt::white);
                for (int i=0; i < datakeys.size(); i++){
                    ui->customplot->addGraph();
                    ui->customplot->graph(i)->setPen(QPen(color[i]));
                    ui->customplot->graph(i)->setName(datakeys[i]);
                    name_line.push_back(datakeys[i]);
                }
            }

            // 添加XData , YData 数据
            XData.push_back(num_point);
            //x的data从0开始


            YData.resize(datakeys.size());

            QVector<double> Data;

            for (int i = 0;  i < name_line.size(); ++i) {
                //obj中的value并且指定key的那个值
               // Data.push_back(jsonObj.value(datakeys[i]).toDouble()*3.3/4096);
                Data.push_back(jsonObj.value(datakeys[i]).toDouble());
            }
            for (int i = 0;  i < name_line.size(); ++i) {
                YData[i].push_back(Data[i]);
            }

            //向坐标值赋值
            //将这次的x和y放上去——index作为次数——i是不同通道的y值对应的位置
            for (int i=0; i < datakeys.size(); ++i){
                ui->customplot->graph(i)->addData(XData[num_point], YData[i][num_point]);
            }
            this->num_point++;

            // 更新坐标
            //使坐标的首位紧贴整个波形，设置跟随的重点在此
            ui->customplot->xAxis->setRange((ui->customplot->graph(0)->dataCount()>300)?
                                                (ui->customplot->graph(0)->dataCount()-300):
                                                0,
                                            ui->customplot->graph(0)->dataCount());
            ui->customplot->replot(QCustomPlot::rpQueuedReplot);  //实现重绘
        }
        // 向接收区打印
        ui->listWidget->addItem(tmpQStr);
        qDebug() << "Received Data: " << tmpQStr;
}

//得出结论，qt画图的性能非常好
void Widget::test()
{
//    qDebug()<<"test";
//    ui->customplot->addGraph();
//    ui->customplot->graph(0)->setName("aaa");
//    double y = 2;
//    for(double i = 0; i < 10000;i++)
//    {
//        ui->customplot->graph(0)->addData(i,y);
//        ui->customplot->replot(QCustomPlot::rpQueuedReplot);//重绘
//        //更新坐标轴，是移动的重点
//        if((int)i % (int)lon == 0)//弹出屏幕外的
//        {
//            //屏幕瞬移
//            ui->customplot->xAxis->setRange(i,i+lon);
//            //已有的线失去
//           // ui->customplot->graph(0)->data().data()->clear();
//        }
//    }


//    //test2 画线的速度是否与串口传输有关 结论 是的
//    qDebug()<<"test2";
//    qDebug()<<num_point;
//    ui->customplot->addGraph();
//    ui->customplot->graph(0)->setName("aaa");
//    double y = 2;
//        ui->customplot->graph(0)->addData(num_point,y);
//        ui->customplot->replot(QCustomPlot::rpQueuedReplot);//重绘
//        //更新坐标轴，是移动的重点
//        if((int)num_point % (int)lon == 0)//弹出屏幕外的
//        {
//            //屏幕瞬移
//            ui->customplot->xAxis->setRange(num_point,num_point+lon);
//            //已有的线失去
//           // ui->customplot->graph(0)->data().data()->clear();
//        }
//    num_point++;

    //test3 串口变慢，是否因为跟画线在一起  //结论，是的 看看能不能把接收和画线分成两个不同的函数
    qDebug()<<"test3";
    qDebug()<<num_point;
    num_point++;
}

void Widget::Is_PopoutSightless()
{
    Is_Popout = !Is_Popout;
}

//接受电压函数——处理成一个单位的json送入到updata——paint出来
void Widget::get_info()
{
    //利用这里的split
   QString data;
   QByteArray info;
   QStringList list_data;

   info = port.readAll();
   qDebug()<<info;
   if(!info.isEmpty())
   {
       if(info.contains('{'))
       {
           data = info;
           list_data = data.split("{");
           if(list_data.size() == 2)
           {
               if(frontData.isEmpty())
               {
                   
                   frontData = list_data.at(1);
                   frontData.insert(0,'{');
                   return;
               }
               oneData = frontData + list_data.at(0);
               frontData = list_data.at(1);
               frontData.insert(0,'{');
                  qDebug()<<oneData;
               update_paint(oneData.toLocal8Bit());
           }
           
           if(list_data.size() == 3)
           {
               //三段数据的处理方式
               oneData = frontData + list_data.at(0);
               frontData = list_data.at(1);
               qDebug()<<oneData;
               update_paint(oneData.toLocal8Bit());
               
               oneData = list_data.at(1);
               oneData.insert(0,'{');
               qDebug()<<oneData;
               update_paint(oneData.toLocal8Bit());
               
               frontData = list_data.at(2);
               frontData.insert(0,'{');
           }
           if(list_data.size() == 4)
           {
               //四段数据的处理方式
               oneData = frontData + list_data.at(0);
               frontData = list_data.at(1);
               qDebug()<<oneData;
               update_paint(oneData.toLocal8Bit());

               //第一段整一段数据
               oneData = list_data.at(1);//上一个32位的余帧
               oneData.insert(0,'{');
               qDebug()<<oneData;
               update_paint(oneData.toLocal8Bit());

               //第二段整段的数据
               oneData = list_data.at(2);
               oneData.insert(0,'{');
               qDebug()<<oneData;
               update_paint(oneData.toLocal8Bit());

               //把此放到
               frontData = list_data.at(2);
               frontData.insert(0,'{');
           }
       }
      
   }
}

void Widget::receiveInfo()//
{
    /*--------------------------------
     *  接受串口信息
     *-------------------------------*/
    QString data;
    QByteArray info = port.readAll();
    QStringList list;

    if(!info.isEmpty())
    {
        if (info.contains('{')){
            data = info;
            list= data.split("{");
            if (list.length() == 2){
                if (frontData.isEmpty()){
                    frontData = list.at(1);
                    frontData.insert(0,'{');
                    return;
                }
                //用于在被截断了的失效数据重现
                oneData = frontData + list.at(0);
                frontData = list.at(1);
                frontData.insert(0,'{');
                plotCustom(oneData.toLocal8Bit());  //绘图
                qDebug() << "One data: " << oneData;
            }
            //因为够了32位就自动进行一次的，顶多是两个，因为传的东西几乎有20个bit
            if (list.length() == 3){
                oneData = frontData + list.at(0);
                plotCustom(oneData.toLocal8Bit());          //绘图
                qDebug() << "One data: " << oneData;
                oneData = list.at(1);
                oneData.insert(0,'{');
                plotCustom(oneData.toLocal8Bit());  //绘图
                qDebug() << "One data: " << oneData;
                frontData = list.at(2);
                frontData.insert(0,'{');
            }
         }
    }
}

//造示波器
void Widget::build_chart()
{
    if(!oscilloscope_ready)
    {
        ui->customplot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom|QCP::iSelectPlottables);
        ui->customplot->yAxis2->setVisible(true);//显示y轴2
        ui->customplot->xAxis2->setVisible(true);//显示x轴2
        //ui->customplot->axisRect()->setupFullAxesBox();
        ui->customplot->yAxis->setRange(0,3.3);
        ui->customplot->yAxis2->setRange(0,3.3);
        if(serial_ready)
        {
            ui->listWidget->addItem("示波器正常打开");
            ui->btn_openocc->setText("关闭示波器");
            oscilloscope_ready = true;
        }
        else
            ui->listWidget->addItem("串口并未正常打开，请检查串口是否打开");
    }
    else
    {
        //标志量置零
        oscilloscope_ready = false;
        num_point = 0;
        name_line.clear();
        XData.clear();
        YData.clear();
        Key_of_Json.clear();
        //图的归位
        ui->customplot->clearGraphs();
        ui->customplot->legend->setVisible(false);
        ui->customplot->replot();
        ui->btn_openocc->setText("打开示波器");
        ui->listWidget->addItem("示波器已关闭");
    }
}

void Widget::buildChart()//
{
    /*--------------------------------
     *  构造示波器
     *-------------------------------*/
    ui->customplot->setBackground(QBrush(QColor("#474848")));
    ui->customplot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);      //可拖拽+可滚轮缩放

    ui->customplot->axisRect()->setupFullAxesBox();

    ui->customplot->yAxis->setRange(0, 3.3);
}

//串口设置函数
void Widget::set_serial()
{

    if(ui->btn_start_end->text() == "关闭串口")
    {
        port.close();
        serial_ready = false;
        ui->listWidget->addItem("串口已关闭");
        ui->btn_start_end->setText("打开串口");
        return;
    }

    //恢复旗帜
    flag_error = 0;

    //设置校验位//
    switch (ui->com_jiao->currentIndex()) {
    case 0:
        port.setParity(QSerialPort::NoParity);
        break;
    case 1:
        port.setParity(QSerialPort::OddParity);
        break;
    case 2:
        port.setParity(QSerialPort::EvenParity);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("校验位设置失败！");
        flag_error = 1;
        break;
    }

    //设置数据位//
    switch (ui->com_shu->currentIndex()) {
    case 0://8
        port.setDataBits(QSerialPort::Data8);
        break;
    case 1://7
        port.setDataBits(QSerialPort::Data7);
        break;
    case 2://6
        port.setDataBits(QSerialPort::Data6);
        break;
    case 3://5
        port.setDataBits(QSerialPort::Data5);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("数据位设置失败！");
        flag_error = 1;
        break;
    }

    //设置停止位//
    switch (ui->com_tin->currentIndex()) {
    case 0://1
        port.setStopBits(QSerialPort::OneStop);
        break;
    case 1://1.5
        port.setStopBits(QSerialPort::OneAndHalfStop);
        break;
    case 2://2
        port.setStopBits(QSerialPort::TwoStop);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("停止位设置失败！");
        flag_error = 1;
        break;
    }

    //设置流控制
    port.setFlowControl(QSerialPort::NoFlowControl);

    //设置端口
    QString name = ui->com_com->currentText();
    QString num;
    //提取出来
    for(int i = 3;i < name.size() ;i++)
    {
      num[i-3] = name[i];
    }
    int ch = num.toInt();
    qDebug()<<ch;
    switch (ch) {
    case 1://1
        port.setPortName("COM1");
        break;
    case 2://1
        port.setPortName("COM2");
        break;
    case 3://1
        port.setPortName("COM3");
        break;
    case 4://1
        port.setPortName("COM4");
        break;
    case 5://1
        port.setPortName("COM5");
        break;
    case 6://1
        port.setPortName("COM6");
        break;
    case 7://1
        port.setPortName("COM7");
        break;
    case 8://1
        port.setPortName("COM8");
        break;
    case 9:
        port.setPortName("COM9");
        break;
    default:
        //输出错误
        ui->listWidget->addItem("端口设置失败！");
        flag_error = 1;
        break;
    }

    //设置波特率
    switch (ui->com_bo->currentIndex()) {
    case 0://115200
        port.setBaudRate(QSerialPort::Baud115200);
        break;
    case 1://9600
        port.setBaudRate(QSerialPort::Baud9600);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("波特率设置失败！");
        flag_error = 1;
        break;
    }

    //设置是否删除不可见部分
    switch (ui->com_ispop->currentIndex()) {
    case 0:
        Is_Popout = 1;
        break;
    default:
        Is_Popout = 0;
        break;
    }


    //与语句，如果前面不满足不执行后边
    if(flag_error == 0 && port.open(QIODevice::ReadWrite))
    {
        //打开串口
        qDebug()<<"串口设置完毕";
        ui->listWidget->addItem("串口已经打开");
        serial_ready = true;

        //打开后，将按钮改一下
        ui->btn_start_end->setText("关闭串口");
    }
    else{
        ui->listWidget->addItem("打开失败，请根据提示进行修复");
    }
}
