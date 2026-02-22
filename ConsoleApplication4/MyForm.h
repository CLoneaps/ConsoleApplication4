
#pragma once

namespace ConsoleApplication4 {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections;
    using namespace System::Collections::Generic;
    using namespace System::Windows::Forms;
    using namespace System::Data;
    using namespace System::Drawing;
    using namespace System::Net;
    using namespace System::Net::Sockets;
    using namespace System::Threading;
    using namespace System::IO;

    // ─────────────────────────────────────────────
    // Хелпер: определение сервиса по номеру порта
    // ─────────────────────────────────────────────
    ref class ServiceHelper {
    public:
        static String^ GetServiceName(int port) {
            switch (port) {
                case 20:   return "FTP-DATA";
                case 21:   return "FTP";
                case 22:   return "SSH";
                case 23:   return "Telnet";
                case 25:   return "SMTP";
                case 53:   return "DNS";
                case 67:   return "DHCP";
                case 80:   return "HTTP";
                case 110:  return "POP3";
                case 111:  return "RPC";
                case 135:  return "MS-RPC";
                case 137:  return "NetBIOS-NS";
                case 138:  return "NetBIOS-DGM";
                case 139:  return "NetBIOS-SSN";
                case 143:  return "IMAP";
                case 161:  return "SNMP";
                case 389:  return "LDAP";
                case 443:  return "HTTPS";
                case 445:  return "SMB";
                case 465:  return "SMTPS";
                case 514:  return "Syslog";
                case 587:  return "SMTP-TLS";
                case 636:  return "LDAPS";
                case 993:  return "IMAPS";
                case 995:  return "POP3S";
                case 1433: return "MSSQL";
                case 1521: return "Oracle-DB";
                case 2222: return "SSH-alt";
                case 3306: return "MySQL";
                case 3389: return "RDP";
                case 5432: return "PostgreSQL";
                case 5900: return "VNC";
                case 6379: return "Redis";
                case 8080: return "HTTP-Proxy";
                case 8443: return "HTTPS-alt";
                case 9200: return "Elasticsearch";
                case 27017: return "MongoDB";
                default:   return "Unknown";
            }
        }
    };

    // ─────────────────────────────────────────────
    // Главная форма
    // ─────────────────────────────────────────────
    public ref class MyForm : public System::Windows::Forms::Form {
    public:
        MyForm(void) {
            InitializeComponent();
            isScanning = false;
            openPorts = gcnew List<String^>();
        }

    protected:
        ~MyForm() {
            if (components) delete components;
        }

    private:
        // ── Контролы ──────────────────────────────
        TabControl^     tabMain;

        // Вкладка: Сканер портов
        TabPage^        tabPorts;
        GroupBox^       grpTarget;
        Label^          lblHost;
        TextBox^        txtHost;
        Label^          lblPortFrom;
        NumericUpDown^  numPortFrom;
        Label^          lblPortTo;
        NumericUpDown^  numPortTo;
        Label^          lblTimeout;
        NumericUpDown^  numTimeout;
        CheckBox^       chkTCP;
        CheckBox^       chkUDP;
        Button^         btnScan;
        Button^         btnStop;
        Button^         btnSave;
        ProgressBar^    progressBar;
        Label^          lblStatus;
        ListView^       listResults;
        ColumnHeader^   colPort;
        ColumnHeader^   colProto;
        ColumnHeader^   colService;
        ColumnHeader^   colStatus;

        // Вкладка: Ping Sweep
        TabPage^        tabPing;
        GroupBox^       grpSubnet;
        Label^          lblSubnet;
        TextBox^        txtSubnet;
        Label^          lblPingTimeout;
        NumericUpDown^  numPingTimeout;
        Button^         btnPingSweep;
        Button^         btnStopPing;
        ListView^       listPingResults;
        ColumnHeader^   colIP;
        ColumnHeader^   colPingStatus;
        ColumnHeader^   colHostname;

        // Вкладка: Лог
        TabPage^        tabLog;
        RichTextBox^    rtbLog;
        Button^         btnClearLog;

        // Нижняя панель
        StatusStrip^    statusStrip;
        ToolStripStatusLabel^ statusLabel;
        ToolStripStatusLabel^ statusFound;

        System::ComponentModel::IContainer^ components;

        // ── Состояние ─────────────────────────────
        bool            isScanning;
        List<String^>^  openPorts;
        int             totalPorts;
        int             scannedPorts;
        int             foundOpen;
        Object^         lockObj = gcnew Object();

        // ─────────────────────────────────────────
        // Делегаты для потокобезопасного UI
        // ─────────────────────────────────────────
        delegate void UpdateProgressDelegate(int value, int total);
        delegate void AddResultDelegate(int port, String^ proto, String^ service, bool open);
        delegate void AddPingResultDelegate(String^ ip, bool alive, String^ hostname);
        delegate void ScanCompleteDelegate(bool stopped);
        delegate void LogDelegate(String^ msg, Color color);

        // ─────────────────────────────────────────
        // InitializeComponent
        // ─────────────────────────────────────────
        void InitializeComponent(void) {
            this->components = gcnew System::ComponentModel::Container();

            // ── Форма ──────────────────────────
            this->Text    = L"PortScanner Pro  |  Учебный инструмент";
            this->Size    = System::Drawing::Size(820, 640);
            this->MinimumSize = System::Drawing::Size(820, 640);
            this->StartPosition = FormStartPosition::CenterScreen;
            this->BackColor = Color::FromArgb(18, 18, 30);
            this->ForeColor = Color::FromArgb(200, 210, 230);
            this->Font     = gcnew System::Drawing::Font("Consolas", 9.0f);

            // ── TabControl ─────────────────────
            tabMain = gcnew TabControl();
            tabMain->Dock = DockStyle::Fill;
            tabMain->Appearance = TabAppearance::FlatButtons;
            tabMain->BackColor  = Color::FromArgb(18, 18, 30);

            tabPorts = gcnew TabPage(L"  Сканер портов  ");
            tabPing  = gcnew TabPage(L"  Ping Sweep  ");
            tabLog   = gcnew TabPage(L"  Лог  ");

            tabPorts->BackColor = Color::FromArgb(18, 18, 30);
            tabPing->BackColor  = Color::FromArgb(18, 18, 30);
            tabLog->BackColor   = Color::FromArgb(18, 18, 30);

            tabMain->TabPages->Add(tabPorts);
            tabMain->TabPages->Add(tabPing);
            tabMain->TabPages->Add(tabLog);

            this->Controls->Add(tabMain);

            // ══════════════════════════════════
            // ВКЛАДКА: СКАНЕР ПОРТОВ
            // ══════════════════════════════════
            grpTarget = gcnew GroupBox();
            grpTarget->Text = "  Цель и параметры";
            grpTarget->ForeColor = Color::FromArgb(100, 200, 255);
            grpTarget->BackColor = Color::FromArgb(25, 25, 40);
            grpTarget->Location = Point(10, 10);
            grpTarget->Size     = System::Drawing::Size(785, 110);

            // Хост
            lblHost = gcnew Label();
            lblHost->Text = "Хост / IP:";
            lblHost->Location = Point(15, 28);
            lblHost->AutoSize = true;

            txtHost = gcnew TextBox();
            txtHost->Text = "127.0.0.1";
            txtHost->Location = Point(110, 25);
            txtHost->Size = System::Drawing::Size(160, 22);
            txtHost->BackColor = Color::FromArgb(35, 35, 55);
            txtHost->ForeColor = Color::FromArgb(100, 255, 150);
            txtHost->BorderStyle = BorderStyle::FixedSingle;

            // Порты
            lblPortFrom = gcnew Label();
            lblPortFrom->Text = "Порт с:";
            lblPortFrom->Location = Point(295, 28);
            lblPortFrom->AutoSize = true;

            numPortFrom = gcnew NumericUpDown();
            numPortFrom->Minimum  = 1;
            numPortFrom->Maximum  = 65535;
            numPortFrom->Value    = 1;
            numPortFrom->Location = Point(360, 25);
            numPortFrom->Size     = System::Drawing::Size(75, 22);
            numPortFrom->BackColor = Color::FromArgb(35, 35, 55);
            numPortFrom->ForeColor = Color::FromArgb(200, 210, 230);

            lblPortTo = gcnew Label();
            lblPortTo->Text = "по:";
            lblPortTo->Location = Point(450, 28);
            lblPortTo->AutoSize = true;

            numPortTo = gcnew NumericUpDown();
            numPortTo->Minimum  = 1;
            numPortTo->Maximum  = 65535;
            numPortTo->Value    = 1024;
            numPortTo->Location = Point(480, 25);
            numPortTo->Size     = System::Drawing::Size(75, 22);
            numPortTo->BackColor = Color::FromArgb(35, 35, 55);
            numPortTo->ForeColor = Color::FromArgb(200, 210, 230);

            // Таймаут
            lblTimeout = gcnew Label();
            lblTimeout->Text = "Таймаут (мс):";
            lblTimeout->Location = Point(575, 28);
            lblTimeout->AutoSize = true;

            numTimeout = gcnew NumericUpDown();
            numTimeout->Minimum  = 50;
            numTimeout->Maximum  = 5000;
            numTimeout->Value    = 500;
            numTimeout->Location = Point(675, 25);
            numTimeout->Size     = System::Drawing::Size(75, 22);
            numTimeout->BackColor = Color::FromArgb(35, 35, 55);
            numTimeout->ForeColor = Color::FromArgb(200, 210, 230);

            // Протоколы
            chkTCP = gcnew CheckBox();
            chkTCP->Text = "TCP";
            chkTCP->Checked = true;
            chkTCP->Location = Point(15, 65);
            chkTCP->ForeColor = Color::FromArgb(100, 200, 255);
            chkTCP->AutoSize = true;

            chkUDP = gcnew CheckBox();
            chkUDP->Text = "UDP (медленнее)";
            chkUDP->Checked = false;
            chkUDP->Location = Point(75, 65);
            chkUDP->ForeColor = Color::FromArgb(255, 180, 80);
            chkUDP->AutoSize = true;

            // Кнопки
            btnScan = gcnew Button();
            btnScan->Text = "▶  СКАНИРОВАТЬ";
            btnScan->Location = Point(450, 58);
            btnScan->Size     = System::Drawing::Size(150, 35);
            btnScan->BackColor = Color::FromArgb(0, 120, 80);
            btnScan->ForeColor = Color::White;
            btnScan->FlatStyle = FlatStyle::Flat;
            btnScan->FlatAppearance->BorderColor = Color::FromArgb(0, 200, 130);
            btnScan->Font = gcnew System::Drawing::Font("Consolas", 9.5f, FontStyle::Bold);
            btnScan->Click += gcnew EventHandler(this, &MyForm::btnScan_Click);

            btnStop = gcnew Button();
            btnStop->Text = "■  СТОП";
            btnStop->Location = Point(615, 58);
            btnStop->Size     = System::Drawing::Size(100, 35);
            btnStop->BackColor = Color::FromArgb(140, 30, 30);
            btnStop->ForeColor = Color::White;
            btnStop->FlatStyle = FlatStyle::Flat;
            btnStop->Enabled  = false;
            btnStop->Click += gcnew EventHandler(this, &MyForm::btnStop_Click);

            btnSave = gcnew Button();
            btnSave->Text = "💾  СОХРАНИТЬ";
            btnSave->Location = Point(728, 58);
            btnSave->Size     = System::Drawing::Size(42, 35);
            btnSave->BackColor = Color::FromArgb(50, 50, 80);
            btnSave->ForeColor = Color::FromArgb(200, 210, 230);
            btnSave->FlatStyle = FlatStyle::Flat;
            btnSave->Click += gcnew EventHandler(this, &MyForm::btnSave_Click);

            grpTarget->Controls->Add(lblHost);
            grpTarget->Controls->Add(txtHost);
            grpTarget->Controls->Add(lblPortFrom);
            grpTarget->Controls->Add(numPortFrom);
            grpTarget->Controls->Add(lblPortTo);
            grpTarget->Controls->Add(numPortTo);
            grpTarget->Controls->Add(lblTimeout);
            grpTarget->Controls->Add(numTimeout);
            grpTarget->Controls->Add(chkTCP);
            grpTarget->Controls->Add(chkUDP);
            grpTarget->Controls->Add(btnScan);
            grpTarget->Controls->Add(btnStop);
            grpTarget->Controls->Add(btnSave);

            // Прогресс
            progressBar = gcnew ProgressBar();
            progressBar->Location = Point(10, 128);
            progressBar->Size     = System::Drawing::Size(785, 18);
            progressBar->Style    = ProgressBarStyle::Continuous;
            progressBar->ForeColor = Color::FromArgb(0, 200, 130);

            lblStatus = gcnew Label();
            lblStatus->Text = "Готов к сканированию";
            lblStatus->Location = Point(10, 150);
            lblStatus->Size     = System::Drawing::Size(785, 18);
            lblStatus->ForeColor = Color::FromArgb(150, 160, 180);

            // Таблица результатов
            listResults = gcnew ListView();
            listResults->Location = Point(10, 172);
            listResults->Size     = System::Drawing::Size(785, 380);
            listResults->View     = View::Details;
            listResults->FullRowSelect = true;
            listResults->GridLines = true;
            listResults->BackColor = Color::FromArgb(12, 12, 22);
            listResults->ForeColor = Color::FromArgb(200, 210, 230);
            listResults->BorderStyle = BorderStyle::FixedSingle;

            colPort    = gcnew ColumnHeader(); colPort->Text    = "Порт";     colPort->Width    = 80;
            colProto   = gcnew ColumnHeader(); colProto->Text   = "Протокол"; colProto->Width   = 90;
            colService = gcnew ColumnHeader(); colService->Text = "Сервис";   colService->Width = 140;
            colStatus  = gcnew ColumnHeader(); colStatus->Text  = "Статус";   colStatus->Width  = 100;

            listResults->Columns->Add(colPort);
            listResults->Columns->Add(colProto);
            listResults->Columns->Add(colService);
            listResults->Columns->Add(colStatus);

            tabPorts->Controls->Add(grpTarget);
            tabPorts->Controls->Add(progressBar);
            tabPorts->Controls->Add(lblStatus);
            tabPorts->Controls->Add(listResults);

            // ══════════════════════════════════
            // ВКЛАДКА: PING SWEEP
            // ══════════════════════════════════
            grpSubnet = gcnew GroupBox();
            grpSubnet->Text = "  Сканирование подсети";
            grpSubnet->ForeColor = Color::FromArgb(255, 180, 80);
            grpSubnet->BackColor = Color::FromArgb(25, 25, 40);
            grpSubnet->Location = Point(10, 10);
            grpSubnet->Size     = System::Drawing::Size(785, 80);

            lblSubnet = gcnew Label();
            lblSubnet->Text = "Подсеть (первые 3 октета):";
            lblSubnet->Location = Point(15, 30);
            lblSubnet->AutoSize = true;

            txtSubnet = gcnew TextBox();
            txtSubnet->Text = "192.168.1.";
            txtSubnet->Location = Point(220, 27);
            txtSubnet->Size = System::Drawing::Size(130, 22);
            txtSubnet->BackColor = Color::FromArgb(35, 35, 55);
            txtSubnet->ForeColor = Color::FromArgb(255, 200, 100);
            txtSubnet->BorderStyle = BorderStyle::FixedSingle;

            lblPingTimeout = gcnew Label();
            lblPingTimeout->Text = "Таймаут (мс):";
            lblPingTimeout->Location = Point(375, 30);
            lblPingTimeout->AutoSize = true;

            numPingTimeout = gcnew NumericUpDown();
            numPingTimeout->Minimum  = 50;
            numPingTimeout->Maximum  = 3000;
            numPingTimeout->Value    = 300;
            numPingTimeout->Location = Point(475, 27);
            numPingTimeout->Size     = System::Drawing::Size(75, 22);
            numPingTimeout->BackColor = Color::FromArgb(35, 35, 55);
            numPingTimeout->ForeColor = Color::FromArgb(200, 210, 230);

            btnPingSweep = gcnew Button();
            btnPingSweep->Text = "▶  PING SWEEP";
            btnPingSweep->Location = Point(575, 22);
            btnPingSweep->Size     = System::Drawing::Size(130, 35);
            btnPingSweep->BackColor = Color::FromArgb(100, 70, 0);
            btnPingSweep->ForeColor = Color::White;
            btnPingSweep->FlatStyle = FlatStyle::Flat;
            btnPingSweep->FlatAppearance->BorderColor = Color::FromArgb(255, 180, 80);
            btnPingSweep->Font = gcnew System::Drawing::Font("Consolas", 9.0f, FontStyle::Bold);
            btnPingSweep->Click += gcnew EventHandler(this, &MyForm::btnPingSweep_Click);

            btnStopPing = gcnew Button();
            btnStopPing->Text = "■";
            btnStopPing->Location = Point(718, 22);
            btnStopPing->Size     = System::Drawing::Size(52, 35);
            btnStopPing->BackColor = Color::FromArgb(140, 30, 30);
            btnStopPing->ForeColor = Color::White;
            btnStopPing->FlatStyle = FlatStyle::Flat;
            btnStopPing->Enabled  = false;
            btnStopPing->Click += gcnew EventHandler(this, &MyForm::btnStopPing_Click);

            grpSubnet->Controls->Add(lblSubnet);
            grpSubnet->Controls->Add(txtSubnet);
            grpSubnet->Controls->Add(lblPingTimeout);
            grpSubnet->Controls->Add(numPingTimeout);
            grpSubnet->Controls->Add(btnPingSweep);
            grpSubnet->Controls->Add(btnStopPing);

            listPingResults = gcnew ListView();
            listPingResults->Location = Point(10, 100);
            listPingResults->Size     = System::Drawing::Size(785, 460);
            listPingResults->View     = View::Details;
            listPingResults->FullRowSelect = true;
            listPingResults->GridLines = true;
            listPingResults->BackColor = Color::FromArgb(12, 12, 22);
            listPingResults->ForeColor = Color::FromArgb(200, 210, 230);
            listPingResults->BorderStyle = BorderStyle::FixedSingle;

            colIP         = gcnew ColumnHeader(); colIP->Text         = "IP-адрес";   colIP->Width         = 160;
            colPingStatus = gcnew ColumnHeader(); colPingStatus->Text = "Статус";     colPingStatus->Width = 120;
            colHostname   = gcnew ColumnHeader(); colHostname->Text   = "Имя хоста"; colHostname->Width   = 300;

            listPingResults->Columns->Add(colIP);
            listPingResults->Columns->Add(colPingStatus);
            listPingResults->Columns->Add(colHostname);

            tabPing->Controls->Add(grpSubnet);
            tabPing->Controls->Add(listPingResults);

            // ══════════════════════════════════
            // ВКЛАДКА: ЛОГ
            // ══════════════════════════════════
            rtbLog = gcnew RichTextBox();
            rtbLog->Dock = DockStyle::Fill;
            rtbLog->BackColor = Color::FromArgb(10, 10, 18);
            rtbLog->ForeColor = Color::FromArgb(180, 190, 210);
            rtbLog->ReadOnly  = true;
            rtbLog->Font      = gcnew System::Drawing::Font("Consolas", 8.5f);
            rtbLog->ScrollBars = RichTextBoxScrollBars::Vertical;

            btnClearLog = gcnew Button();
            btnClearLog->Text = "Очистить лог";
            btnClearLog->Dock = DockStyle::Bottom;
            btnClearLog->Height = 28;
            btnClearLog->BackColor = Color::FromArgb(40, 40, 60);
            btnClearLog->ForeColor = Color::FromArgb(200, 210, 230);
            btnClearLog->FlatStyle = FlatStyle::Flat;
            btnClearLog->Click += gcnew EventHandler(this, &MyForm::btnClearLog_Click);

            tabLog->Controls->Add(rtbLog);
            tabLog->Controls->Add(btnClearLog);

            // ══════════════════════════════════
            // STATUS BAR
            // ══════════════════════════════════
            statusStrip = gcnew StatusStrip();
            statusStrip->BackColor = Color::FromArgb(12, 12, 22);

            statusLabel = gcnew ToolStripStatusLabel();
            statusLabel->Text = "PortScanner Pro  |  Только для учебных целей";
            statusLabel->ForeColor = Color::FromArgb(100, 110, 140);

            statusFound = gcnew ToolStripStatusLabel();
            statusFound->Text = "Открытых портов: 0";
            statusFound->ForeColor = Color::FromArgb(0, 200, 130);
            statusFound->Spring = true;
            statusFound->TextAlign = ContentAlignment::MiddleRight;

            statusStrip->Items->Add(statusLabel);
            statusStrip->Items->Add(statusFound);
            this->Controls->Add(statusStrip);

            // Приветственный лог
            AppendLog("=== PortScanner Pro — Учебный инструмент ===", Color::FromArgb(100, 200, 255));
            AppendLog("Используй только на своём оборудовании или с разрешения владельца.", Color::FromArgb(255, 180, 80));
            AppendLog("", Color::White);
        }

        // ─────────────────────────────────────────
        // ЛОГ
        // ─────────────────────────────────────────
        void AppendLog(String^ msg, Color color) {
            if (rtbLog->InvokeRequired) {
                rtbLog->Invoke(gcnew LogDelegate(this, &MyForm::AppendLog), msg, color);
                return;
            }
            rtbLog->SelectionStart  = rtbLog->TextLength;
            rtbLog->SelectionLength = 0;
            rtbLog->SelectionColor  = color;
            rtbLog->AppendText(DateTime::Now.ToString("[HH:mm:ss] ") + msg + "\n");
            rtbLog->ScrollToCaret();
        }

        void btnClearLog_Click(Object^ sender, EventArgs^ e) {
            rtbLog->Clear();
        }

        // ─────────────────────────────────────────
        // СКАНЕР ПОРТОВ — логика
        // ─────────────────────────────────────────

        // Параметры сканирования (передаём в поток через object array)
        ref struct ScanParams {
            String^ host;
            int portFrom, portTo, timeoutMs;
            bool tcp, udp;
        };

        volatile bool stopFlag;

        void btnScan_Click(Object^ sender, EventArgs^ e) {
            if (isScanning) return;

            String^ host = txtHost->Text->Trim();
            if (String::IsNullOrEmpty(host)) {
                MessageBox::Show("Введите хост или IP-адрес.", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }
            if (!chkTCP->Checked && !chkUDP->Checked) {
                MessageBox::Show("Выбери хотя бы один протокол (TCP / UDP).", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            int pFrom = (int)numPortFrom->Value;
            int pTo   = (int)numPortTo->Value;
            if (pFrom > pTo) {
                MessageBox::Show("Начальный порт не может быть больше конечного.", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            listResults->Items->Clear();
            openPorts->Clear();
            foundOpen = 0;
            totalPorts = (pTo - pFrom + 1) * ((chkTCP->Checked ? 1 : 0) + (chkUDP->Checked ? 1 : 0));
            scannedPorts = 0;
            progressBar->Maximum = totalPorts;
            progressBar->Value   = 0;
            isScanning = true;
            stopFlag   = false;

            btnScan->Enabled = false;
            btnStop->Enabled = true;

            AppendLog("Запуск сканирования: " + host + " портов " + pFrom + "-" + pTo, Color::FromArgb(100, 200, 255));

            ScanParams^ p = gcnew ScanParams();
            p->host     = host;
            p->portFrom = pFrom;
            p->portTo   = pTo;
            p->timeoutMs= (int)numTimeout->Value;
            p->tcp      = chkTCP->Checked;
            p->udp      = chkUDP->Checked;

            Thread^ t = gcnew Thread(gcnew ParameterizedThreadStart(this, &MyForm::ScanWorker));
            t->IsBackground = true;
            t->Start(p);
        }

        void btnStop_Click(Object^ sender, EventArgs^ e) {
            stopFlag = true;
            AppendLog("Остановка сканирования...", Color::FromArgb(255, 100, 100));
        }

        void ScanWorker(Object^ param) {
            ScanParams^ p = safe_cast<ScanParams^>(param);

            for (int port = p->portFrom; port <= p->portTo; port++) {
                if (stopFlag) break;

                if (p->tcp) {
                    bool open = ScanTCP(p->host, port, p->timeoutMs);
                    UpdateProgress(++scannedPorts, totalPorts);
                    if (open) {
                        String^ svc = ServiceHelper::GetServiceName(port);
                        AddResult(port, "TCP", svc, true);
                        AppendLog("  [OPEN]  " + port + "/tcp  " + svc, Color::FromArgb(0, 230, 140));
                        Monitor::Enter(lockObj);
                        openPorts->Add(port + "/TCP/" + svc);
                        foundOpen++;
                        Monitor::Exit(lockObj);
                    }
                }

                if (stopFlag) break;

                if (p->udp) {
                    bool open = ScanUDP(p->host, port, p->timeoutMs);
                    UpdateProgress(++scannedPorts, totalPorts);
                    if (open) {
                        String^ svc = ServiceHelper::GetServiceName(port);
                        AddResult(port, "UDP", svc, true);
                        AppendLog("  [OPEN?] " + port + "/udp  " + svc, Color::FromArgb(255, 200, 80));
                        Monitor::Enter(lockObj);
                        openPorts->Add(port + "/UDP/" + svc);
                        foundOpen++;
                        Monitor::Exit(lockObj);
                    }
                }
            }

            ScanComplete(stopFlag);
        }

        // TCP connect scan
        bool ScanTCP(String^ host, int port, int timeoutMs) {
            try {
                TcpClient^ client = gcnew TcpClient();
                IAsyncResult^ ar = client->BeginConnect(host, port, nullptr, nullptr);
                bool connected   = ar->AsyncWaitHandle->WaitOne(timeoutMs, false);
                if (connected && client->Connected) {
                    client->Close();
                    return true;
                }
                client->Close();
            } catch (...) {}
            return false;
        }

        // UDP scan (отправляем пустой датаграмм, смотрим на ICMP unreachable)
        bool ScanUDP(String^ host, int port, int timeoutMs) {
            try {
                UdpClient^ udp = gcnew UdpClient();
                udp->Client->ReceiveTimeout = timeoutMs;
                udp->Connect(host, port);
                array<Byte>^ data = gcnew array<Byte>(1) { 0x00 };
                udp->Send(data, data->Length);
                IPEndPoint^ ep = gcnew IPEndPoint(IPAddress::Any, 0);
                try {
                    udp->Receive(ep);
                    udp->Close();
                    return true;  // получили ответ — порт открыт
                } catch (SocketException^ se) {
                    udp->Close();
                    // ICMP unreachable = порт закрыт
                    return (se->SocketErrorCode != SocketError::ConnectionReset &&
                            se->SocketErrorCode != SocketError::ConnectionRefused);
                }
            } catch (...) {}
            return false;
        }

        // ─────────────────────────────────────────
        // Потокобезопасное обновление UI
        // ─────────────────────────────────────────
        void UpdateProgress(int value, int total) {
            if (progressBar->InvokeRequired) {
                progressBar->Invoke(gcnew UpdateProgressDelegate(this, &MyForm::UpdateProgress), value, total);
                return;
            }
            if (value <= progressBar->Maximum) progressBar->Value = value;
            int pct = total > 0 ? (value * 100 / total) : 0;
            lblStatus->Text = String::Format("Сканирование... {0}/{1} ({2}%)  |  Открыто: {3}", value, total, pct, foundOpen);
            statusFound->Text = "Открытых портов: " + foundOpen;
        }

        void AddResult(int port, String^ proto, String^ service, bool open) {
            if (listResults->InvokeRequired) {
                listResults->Invoke(gcnew AddResultDelegate(this, &MyForm::AddResult), port, proto, service, open);
                return;
            }
            ListViewItem^ item = gcnew ListViewItem(port.ToString());
            item->SubItems->Add(proto);
            item->SubItems->Add(service);
            item->SubItems->Add(open ? "OPEN" : "closed");
            item->ForeColor = open ? Color::FromArgb(0, 230, 140) : Color::FromArgb(140, 140, 160);
            item->BackColor = open ? Color::FromArgb(0, 40, 25)   : Color::FromArgb(12, 12, 22);
            listResults->Items->Add(item);
        }

        void ScanComplete(bool stopped) {
            if (this->InvokeRequired) {
                this->Invoke(gcnew ScanCompleteDelegate(this, &MyForm::ScanComplete), stopped);
                return;
            }
            isScanning = false;
            btnScan->Enabled = true;
            btnStop->Enabled = false;
            progressBar->Value = stopped ? progressBar->Value : progressBar->Maximum;

            if (stopped)
                lblStatus->Text = "Сканирование остановлено. Открытых портов: " + foundOpen;
            else
                lblStatus->Text = "Сканирование завершено. Открытых портов: " + foundOpen;

            statusFound->Text = "Открытых портов: " + foundOpen;
            AppendLog("=== " + (stopped ? "Остановлено" : "Завершено") + ". Открыто: " + foundOpen + " ===",
                      Color::FromArgb(100, 200, 255));
        }

        // ─────────────────────────────────────────
        // СОХРАНЕНИЕ РЕЗУЛЬТАТОВ
        // ─────────────────────────────────────────
        void btnSave_Click(Object^ sender, EventArgs^ e) {
            SaveFileDialog^ dlg = gcnew SaveFileDialog();
            dlg->Filter   = "Text files (*.txt)|*.txt|All files (*.*)|*.*";
            dlg->FileName = "scan_results_" + DateTime::Now.ToString("yyyyMMdd_HHmmss") + ".txt";
            if (dlg->ShowDialog() == Windows::Forms::DialogResult::OK) {
                StreamWriter^ sw = gcnew StreamWriter(dlg->FileName);
                sw->WriteLine("=== PortScanner Pro — Результаты ===");
                sw->WriteLine("Хост: " + txtHost->Text);
                sw->WriteLine("Дата: " + DateTime::Now.ToString());
                sw->WriteLine("Открытых портов: " + openPorts->Count);
                sw->WriteLine("--------------------------------------");
                for each (String^ line in openPorts)
                    sw->WriteLine(line);
                sw->Close();
                AppendLog("Результаты сохранены: " + dlg->FileName, Color::FromArgb(0, 200, 130));
            }
        }

        // ─────────────────────────────────────────
        // PING SWEEP — логика
        // ─────────────────────────────────────────
        ref struct PingParams {
            String^ subnet;
            int timeoutMs;
        };

        volatile bool stopPingFlag;

        void btnPingSweep_Click(Object^ sender, EventArgs^ e) {
            if (isScanning) return;

            String^ subnet = txtSubnet->Text->Trim();
            if (!subnet->EndsWith(".")) subnet += ".";

            listPingResults->Items->Clear();
            isScanning     = true;
            stopPingFlag   = false;
            btnPingSweep->Enabled = false;
            btnStopPing->Enabled  = true;

            AppendLog("Ping sweep: " + subnet + "1-254", Color::FromArgb(255, 200, 80));

            PingParams^ p = gcnew PingParams();
            p->subnet    = subnet;
            p->timeoutMs = (int)numPingTimeout->Value;

            Thread^ t = gcnew Thread(gcnew ParameterizedThreadStart(this, &MyForm::PingWorker));
            t->IsBackground = true;
            t->Start(p);
        }

        void btnStopPing_Click(Object^ sender, EventArgs^ e) {
            stopPingFlag = true;
        }

        void PingWorker(Object^ param) {
            PingParams^ p = safe_cast<PingParams^>(param);
            int alive = 0;

            for (int i = 1; i <= 254; i++) {
                if (stopPingFlag) break;

                String^ ip = p->subnet + i;
                try {
                    System::Net::NetworkInformation::Ping^ ping =
                        gcnew System::Net::NetworkInformation::Ping();
                    System::Net::NetworkInformation::PingReply^ reply =
                        ping->Send(ip, p->timeoutMs);

                    bool isAlive = (reply->Status ==
                        System::Net::NetworkInformation::IPStatus::Success);

                    String^ hostname = "";
                    if (isAlive) {
                        try {
                            IPHostEntry^ entry = Dns::GetHostEntry(ip);
                            hostname = entry->HostName;
                        } catch (...) {
                            hostname = "(нет DNS)";
                        }
                        alive++;
                        AppendLog("  [ALIVE] " + ip + "  " + hostname, Color::FromArgb(0, 230, 140));
                    }

                    AddPingResult(ip, isAlive, hostname);
                } catch (...) {
                    AddPingResult(ip, false, "");
                }
            }

            // Завершение
            this->Invoke(gcnew Action(this, &MyForm::PingSweepComplete));
        }

        void AddPingResult(String^ ip, bool alive, String^ hostname) {
            if (listPingResults->InvokeRequired) {
                listPingResults->Invoke(gcnew AddPingResultDelegate(this, &MyForm::AddPingResult), ip, alive, hostname);
                return;
            }
            if (!alive) return;  // показываем только живые хосты
            ListViewItem^ item = gcnew ListViewItem(ip);
            item->SubItems->Add(alive ? "● ALIVE" : "○ нет");
            item->SubItems->Add(hostname);
            item->ForeColor = Color::FromArgb(0, 230, 140);
            item->BackColor = Color::FromArgb(0, 35, 20);
            listPingResults->Items->Add(item);
        }

        void PingSweepComplete() {
            isScanning = false;
            btnPingSweep->Enabled = true;
            btnStopPing->Enabled  = false;
            AppendLog("=== Ping sweep завершён ===", Color::FromArgb(255, 200, 80));
        }
    };
}
