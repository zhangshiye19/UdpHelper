#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QStringList>
#include <QDebug>
#include <QUdpSocket>
#include <QStandardItemModel>
#include <QFileDialog>
#include <math.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // 添加数据group box表格字段
    void addDataTableField(const QString &fieldName, const QString &fieldType, const QString &fieldValue,
                           const QString &fieldLength);
    void delDataTableField(int row);
    QByteArray readDataTableToMsg();
    void writeDataTableFromMsg();

    // 添加接收到的udp数据
    void addLogToMsgList(const QByteArray &data, quint64 length, const QString &fromIP, const QString &fromPort,
                         const QString &toIP, const QString &toPort);

    // 展示消息详情
    void setMsgText(const QByteArray &msg);

    void loadDataTemplate(const QString &filename);

    void saveDataTemplate(const QString &filename);



private slots:
    void on_dataButtonAddField_clicked();

    void on_dataButtonDelField_clicked();

    void on_dataButtonSave_clicked();

    void on_dataButtonLoad_clicked();

    void on_buttonSend_clicked();

    void on_configButtonApply_clicked();

    void on_dataButtonReader_clicked();

    void on_dataButtonWriter_clicked();

    // 计算所有字节数
    void countDataBytes();

    void onReceiveUdpMsg();

    void sendUdpMsg(const QByteArray& data);

    void onClickedMsgTable(const QModelIndex &index);

    void setConfigText();

    void countTotalBytesNum();

private:
    Ui::MainWindow *ui;
    QByteArray curMsgBytes;
    QUdpSocket *m_udpS;
    QStandardItemModel *msgTableMode;
    QString remote_ip = "127.0.0.1";
    quint16 remote_port = 10001;
    quint16 loc_port = 10001;
    QList<QByteArray> msgBytesList;

    void setUdpBind();
};
#endif // MAINWINDOW_H
