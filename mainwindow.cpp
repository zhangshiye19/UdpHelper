#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    QObject::connect(ui->dataTableWidget, &QTableWidget::itemChanged, this, &MainWindow::countDataBytes);

    // udp
    m_udpS = new QUdpSocket(this);
    m_udpS->bind(QHostAddress::Any, this->remote_port, QUdpSocket::ShareAddress);
    QObject::connect(this->m_udpS, &QUdpSocket::readyRead, this, &MainWindow::onReceiveUdpMsg);

    // 配置IP和端口
//    this->remote_ip = ui->configLineEditIP->text();
//    this->remote_port = ui->configLineEditPort->text().toUShort();
    applyConfig();

    // msg table
    msgListMode = new QStandardItemModel();
//    msgListMode->rea
    msgListMode->setColumnCount(3);
    msgListMode->setHeaderData(0, Qt::Horizontal, "FROM");
    msgListMode->setHeaderData(1, Qt::Horizontal, "TO");
    msgListMode->setHeaderData(2, Qt::Horizontal, "LENGTH");
    ui->msgTable->setModel(msgListMode);
//    ui->msgTable->resizeColumnsToContents();    // 自动调整列宽度
    ui->msgTable->horizontalHeader()->setStretchLastSection(true);  // 最后一个填充空白
    ui->msgTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QObject::connect(ui->msgTable, &QTableView::doubleClicked, this, &MainWindow::onClickedMsgTable);
}


void MainWindow::addDataTableField(const QString &fieldName, const QString &fieldType, const QString &fieldValue,
                                   const QString &fieldLength) {
    const int row = ui->dataTableWidget->rowCount();
    ui->dataTableWidget->insertRow(row);

    ui->dataTableWidget->setItem(row, 0, new QTableWidgetItem(fieldName));

    // 下拉框，选择数据类型
    QComboBox *combobox = new QComboBox();
    // 数据类型
    QStringList typeList{"int64", "int32", "int16", "char", "uint8", "uint16", "uint32", "uint64", "float", "double"};
    combobox->addItems(typeList);
    combobox->setCurrentText(fieldType);
    ui->dataTableWidget->setCellWidget(row, 1, combobox);

    // 数值
    ui->dataTableWidget->setItem(row, 2, new QTableWidgetItem(fieldValue));

    ui->dataTableWidget->setItem(row, 3, new QTableWidgetItem(fieldLength));
}

void MainWindow::delDataTableField(int row) {
    ui->dataTableWidget->removeRow(row);
}

QByteArray MainWindow::readTableData() {
    const int rowCount = ui->dataTableWidget->rowCount();
    QByteArray bytes;
    for (int row = 0; row < rowCount; row++) {
        QComboBox *combobox = dynamic_cast<QComboBox *>(ui->dataTableWidget->cellWidget(row, 1));
        QString text = ui->dataTableWidget->item(row, 2)->text();
        QString comboboxSelectedText = combobox->currentText();
        if (comboboxSelectedText == "int64") {
            std::int64_t long_data = text.toLongLong();
            bytes.append((char *) &long_data, sizeof long_data);
        } else if (comboboxSelectedText == "int32") {
            std::int32_t int_data = text.toInt();
            bytes.append((char *) &int_data, sizeof int_data);
        } else if (comboboxSelectedText == "int16") {
            std::int16_t short_data = text.toShort();
            bytes.append((char *) &short_data, sizeof short_data);
        } else if (comboboxSelectedText == "char" || comboboxSelectedText == "uint8") {   // 有争议
            size_t length = ui->dataTableWidget->item(row, 3)->text().toULongLong(); // 长度
            QStringList list = text.split(';'); // 通过;分割

            for (size_t i = 0; i < length; i++) {
                if (i < list.length()) {    // 实际填写少于长度，按照长度来算
                    char char_data = (char) list[i].toUInt();
                    bytes.append(char_data);
                } else {
                    bytes.append('\0');
                }
            }
        } else if (comboboxSelectedText == "double") {
            std::double_t double_data = text.toDouble();
            bytes.append((char *) &double_data, sizeof double_data);
        } else if (comboboxSelectedText == "float") {
            std::float_t float_data = text.toFloat();
            bytes.append((char *) &float_data, sizeof float_data);
        } else if (comboboxSelectedText == "uint16") {
            std::uint16_t unsigned_short_data = text.toUShort();
            bytes.append((char *) &unsigned_short_data, sizeof unsigned_short_data);
        } else if (comboboxSelectedText == "uint32") {
            std::uint32_t unsigned_int = text.toUInt();
            bytes.append((char *) &unsigned_int, sizeof unsigned_int);
        } else if (comboboxSelectedText == "uint64") {
            std::uint64_t unsigned_long = text.toULongLong();
            bytes.append((char *) &unsigned_long, sizeof unsigned_long);
        }
    }

    return bytes;
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::on_dataButtonAddField_clicked() {
    this->addDataTableField("", "uint16", "", "1");
}

void MainWindow::setMsgText(const QByteArray &msg) {
    this->curMsgBytes = msg;
    QString text = QString::fromStdString(msg.toHex(' ').toStdString());
    ui->msgText->clear();
    ui->msgText->append(text);
}


void MainWindow::on_dataButtonDelField_clicked() {
    const int row = ui->dataTableWidget->currentRow();
    this->delDataTableField(row);
}


void MainWindow::on_dataButtonSave_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, tr("载入文件"), "", "*.json");
    if (filename.isEmpty()) {
        return;
    } else {
        this->saveDataTemplate(filename);
    }
}


void MainWindow::on_dataButtonLoad_clicked() {
    QString filename = QFileDialog::getOpenFileName(this, "打开文件", "", "*.json");
    if (filename.isEmpty()) {
        return;
    } else {
        this->loadDataTemplate(filename);
    }
}


void MainWindow::on_buttonSend_clicked() {
    this->sendUdpMsg(this->curMsgBytes);
}


void MainWindow::on_configButtonApply_clicked() {
    this->remote_ip = ui->configLineEditIP->text();
    this->remote_port = ui->configLineEditPort->text().toUShort();

    applyConfig();
}


void MainWindow::on_dataButtonReader_clicked() {
    this->writeTableData();
}

// 数据写入到消息框
void MainWindow::on_dataButtonWriter_clicked() {
    QByteArray bytes = this->readTableData();
    this->setMsgText(bytes);
}

// 数据写入到表
void MainWindow::writeTableData() {
    size_t offset = 0;
    size_t size = this->curMsgBytes.size();
    char *data = this->curMsgBytes.data();
    int rowCount = ui->dataTableWidget->rowCount();
    for (int row = 0; row < rowCount; row++) {
        // 字节数太少则填充前面的
        if (offset >= this->curMsgBytes.size()) {
            ui->dataTableWidget->item(row, 2)->setText("");
            continue;
        }

        QComboBox *combobox = dynamic_cast<QComboBox *>(ui->dataTableWidget->cellWidget(row, 1));
        QString comboboxSelectedText = combobox->currentText();

//        std::unordered_map<std::string ,class T> typeMap;
        if (comboboxSelectedText == "int64") {
            std::int64_t *value = (std::int64_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "int32") {
            std::int32_t *value = (std::int32_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "int16") {
            std::int16_t *value = (std::int16_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "char" || comboboxSelectedText == "uint8") {
            //
            size_t length = ui->dataTableWidget->item(row, 3)->text().toULongLong();
            QStringList chs;
            for (int i = 0; i < length; ++i) {
                std::uint8_t ch = *(std::uint8_t *) (data + offset + i);
                chs.push_back(QString::number(ch));
            }
            offset += (sizeof(std::uint8_t) * length);
            ui->dataTableWidget->item(row, 2)->setText(chs.join(";"));
        } else if (comboboxSelectedText == "double") {
            std::double_t *value = (std::double_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "float") {
            std::float_t *value = (std::float_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "uint16") {
            std::uint16_t *value = (std::uint16_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "uint32") {
            std::uint32_t *value = (std::uint32_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        } else if (comboboxSelectedText == "uint64") {
            std::uint64_t *value = (std::uint64_t *) (data + offset);
            offset += sizeof *value;
            ui->dataTableWidget->item(row, 2)->setText(QString::number(*value));
        }

    }
}

void
MainWindow::addLogToMsgList(const QByteArray &data, quint64 length, const QString &fromIP, const QString &fromPort,
                            const QString &toIP, const QString &toPort) {
    const int rowCount = ui->msgTable->model()->rowCount();
    this->msgBytesList.append(data);
    QList<QStandardItem *> info{new QStandardItem(fromIP),
                                new QStandardItem(fromPort),
                                new QStandardItem(QString::number(length))};
    msgListMode->insertRow(rowCount, info);
}

void MainWindow::countDataBytes() {
    int rowCount = ui->dataTableWidget->rowCount();
    int size = 0;
    // 略过
}

void MainWindow::onReceiveUdpMsg() {
    while (this->m_udpS->hasPendingDatagrams()) {
        QByteArray dataGram;
        dataGram.resize((int) m_udpS->pendingDatagramSize());
        QHostAddress address;
        quint16 port;
        m_udpS->readDatagram(dataGram.data(), dataGram.size(), &address, &port);

        // 收到消息
        // 将数据传送给msgTable
        this->addLogToMsgList(dataGram, dataGram.size(),
                              QHostAddress(address.toIPv4Address()).toString(),
                              QString::number(port),
                              "127.0.0.1",
                              "10001");
    }
}

void MainWindow::sendUdpMsg(const QByteArray &data) {
    this->m_udpS->writeDatagram(data, QHostAddress(this->remote_ip), this->remote_port);
    this->addLogToMsgList(data, data.size(),
                          "127.0.0.1",
                          "10001",
                          this->remote_ip,
                          QString::number(this->remote_port));
}

void MainWindow::onClickedMsgTable(const QModelIndex &index) {
//    QStandardItemModel *msg_model = dynamic_cast<QStandardItemModel *>(ui->msgTable->msgListMode());
    int cur_row = index.row();
    QByteArray data = msgBytesList.at(cur_row);
    this->setMsgText(data);
}

// 写出数据模板
void MainWindow::saveDataTemplate(const QString &filename) {
    QJsonArray qJsonArray;
    int rowCount = ui->dataTableWidget->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        QJsonObject item;
        QString dName = ui->dataTableWidget->item(i, 0)->text();
        QComboBox *combobox = dynamic_cast<QComboBox *>(ui->dataTableWidget->cellWidget(i, 1));
        QString dType = combobox->currentText();
        QString dValue = ui->dataTableWidget->item(i, 2)->text();
        QString dLength = ui->dataTableWidget->item(i, 3)->text();

        item.insert("name", dName);
        item.insert("type", dType);
        item.insert("value", dValue);
        item.insert("length", dLength);
        qJsonArray.append(item);
    }
    // 文件
    QFile qFile(filename);
    if (qFile.open(QIODevice::ReadWrite)) {
        QJsonDocument document(qJsonArray);
        qFile.write(document.toJson());
    }
    qFile.close();
}

// 载入数据模板
void MainWindow::loadDataTemplate(const QString &filename) {
    ui->dataTableWidget->clear();

    QFile qFile(filename);
    if (!qFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray bytes = qFile.readAll();

    QJsonDocument document = QJsonDocument::fromJson(bytes);
    QJsonArray qJsonArray = document.array();
    for (int i = 0; i < qJsonArray.size(); ++i) {
        QJsonObject item = qJsonArray.at(i).toObject();
        QString dName = item.value("name").toString();
        QString dType = item.value("type").toString();
        QString dValue = item.value("value").toString();
        QString dLength = item.value("length").toString();

        this->addDataTableField(dName, dType, dValue, dLength);
    }

    qFile.close();
}

void MainWindow::applyConfig() {
    ui->configLineEditIP->setText(this->remote_ip);
    ui->configLineEditPort->setText(QString::number(this->remote_port));
}
