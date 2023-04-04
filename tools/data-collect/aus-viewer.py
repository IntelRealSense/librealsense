import sys
from PyQt5.QtWidgets import *
from PyQt5.QtChart import *
from PyQt5.QtGui import *
from PyQt5.QtCore import Qt
#import pyrealsense2 as rs
#import json

dictionary = {
        'RS2_AUS_CONNECTED_DEVICES_COUNTER': '1',
        'RS2_AUS_DEPTH_VISUALIZATION_COLORIZED_FRAMES_COUNTER': '-1',
        'RS2_AUS_INTEL_REALSENSE_D455_CONNECTED_DEVICES_COUNTER': '3',
        'RS2_AUS_INTEL_REALSENSE_D435I_CONNECTED_DEVICES_COUNTER': '2',
        'RS2_AUS_INTEL_REALSENSE_D415_CONNECTED_DEVICES_COUNTER': '1',
        'RS2_AUS_INTEL_REALSENSE_D457_CONNECTED_DEVICES_COUNTER': '1',
        'RS2_AUS_INTEL_REALSENSE_D455_DEPTH_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D455_GYRO_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D455_COLOR_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D455_ACCEL_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D435I_DEPTH_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D435I_GYRO_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D435I_COLOR_TIMER': '1',
        'RS2_AUS_INTEL_REALSENSE_D435I_ACCEL_TIMER': '1',
        'os_name': 'windows',
        'platform_name': 'amd',
        'librealsense_version': '2.5.4'
    }

class AUS(QMainWindow):
    def __init__(self):
        super().__init__()
        self.title = 'AUS Viewer'
        self.left = 10
        self.top = 100
        self.width = 540
        self.height = 550
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        self.initUI()

    def initUI(self):
        grid = QGridLayout(self.centralWidget())
        grid.addWidget(self.technical_info(), 0, 0)
        grid.addWidget(self.add_counters_data(), 0, 1)
        grid.addWidget(self.add_timers_data(), 0, 2)
        self.on_click_display_timer()
        self.on_click_display_counter()

        self.setLayout(grid)
        self.setWindowTitle(self.title)
        self.setGeometry(self.left, self.top, self.width, self.height)
        self.show()

    def technical_info(self):
        groupBox = QGroupBox("Technical info")
        groupBox.setFont(QFont('Times', 8))
        os_name_text = QLabel(self)
        os_name_text.setFont(QFont('Times', 8))
        os_name_text.setText('OS:')
        self.os_name = QLineEdit(self)
        self.os_name .setFont(QFont('Times', 8))
        self.os_name.setText(dictionary['os_name'])
        self.os_name.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.os_name.setReadOnly(True)
        platform_name_text = QLabel(self)
        platform_name_text.setText('Platform:')
        platform_name_text.setFont(QFont('Times', 8))
        self.platform_name = QLineEdit(self)
        self.platform_name.setFont(QFont('Times', 8))
        self.platform_name.setText(dictionary['platform_name'])
        self.platform_name.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.platform_name.setReadOnly(True)
        librealsense_version_text = QLabel(self)
        librealsense_version_text.setText('Librealsense version:')
        librealsense_version_text.setFont(QFont('Times', 8))
        self.librealsense_version = QLineEdit(self)
        self.librealsense_version.setFont(QFont('Times', 8))
        self.librealsense_version.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.librealsense_version.setText(dictionary['librealsense_version'])
        self.librealsense_version.setReadOnly(True)
        vbox = QVBoxLayout()
        vbox.addWidget(os_name_text)
        vbox.addWidget(self.os_name)
        vbox.addWidget(platform_name_text)
        vbox.addWidget(self.platform_name)
        vbox.addWidget(librealsense_version_text)
        vbox.addWidget(self.librealsense_version)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)
        return groupBox

    def add_timers_data(self):
        groupBox = QGroupBox("Timers")
        groupBox.setFont(QFont('Times', 8))
        self.button_display = QPushButton('Display graph', self)
        self.button_display.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.button_display.setToolTip('Display')
        self.button_display.clicked.connect(self.on_click_display_timer)

        self.button_reset = QPushButton('Reset', self)
        self.button_reset.setToolTip('Reset')
        self.button_reset.clicked.connect(self.on_click_reset_timer)

        graph_type_text = QLabel(self)
        graph_type_text.setText('Graph type:')
        self.graph_type_combo = QComboBox(self)
        self.graph_type_combo.addItems(['Bar', 'Pie'])

        self.button_table_timers = QPushButton('Display table', self)
        self.button_table_timers.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.button_table_timers.setToolTip('Table')
        self.button_table_timers.clicked.connect(self.on_click_table_timer)

        self.chart_timers = QChart()
        self.chart_timers.setBackgroundBrush(QBrush(QColor("transparent")))
        self.chart_view_timers = QChartView(self.chart_timers)
        self.chart_view_timers.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.chart_view_timers.setMinimumSize(500, 500)

        self.table_timers = QTableWidget()
        self.table_timers.setStyleSheet("background-color: transparent; border-style: none;")
        self.table_timers.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.table_timers.setMinimumSize(200, 200)

        vbox = QVBoxLayout()
        vbox.addWidget(graph_type_text)
        vbox.addWidget(self.graph_type_combo)
        vbox.addWidget(self.button_display)
        vbox.addWidget(self.button_reset)
        vbox.addWidget(self.button_table_timers)
        vbox.addWidget(self.chart_view_timers)
        vbox.addWidget(self.table_timers)

        vbox.addStretch(1)
        groupBox.setLayout(vbox)
        return groupBox

    def on_click_display_timer(self):
        graph_type = self.graph_type_combo.currentText()
        self.chart_timers.removeAllSeries()
        self.chart_timers.show()
        xAxis = [(key[24:-6:]) for key, value in dictionary.items() if
                 ('TIMER' in key)]
        yAxis = [int(value) for key, value in dictionary.items() if
                 ('TIMER' in key)]
        if graph_type == "Bar":
            self.chart_timers.setTitle('Bar Graph')
            devices_names = []
            [devices_names.append(item.split("_")[0]) for item in xAxis if
             item.split("_")[0] not in devices_names]
            self.categories = []
            self.values = []
            self.sets = []
            for name in devices_names:
                xAxis_sorted = [(key[24::]) for key, value in dictionary.items() if name in key and 'TIMER' in key]
                yAxis_sorted = [int(value) for key,value in dictionary.items() if name in key and 'TIMER' in key]
                [self.categories.append(item.split("_")[1]) for item in xAxis_sorted if
                 item.split("_")[1] not in self.categories]
                self.values.append([yAxis_sorted])
                self.sets.append(QBarSet(name))
            for i in range(len(self.sets)):
                for value in self.values[i]:
                    self.sets[i].append(value)
            self.series = QBarSeries()
            for set in self.sets:
                self.series.append(set)
            self.chart_timers.addSeries(self.series)
            axis_x = QBarCategoryAxis()
            axis_x.append(self.categories)
            self.chart_timers.createDefaultAxes()
            self.chart_timers.axisY(self.series).setTitleText("Seconds")
            self.chart_timers.setAxisX(axis_x, self.series)
            # self.chart_timers.axisX(self.series).setTitleText("Sensor")
            self.chart_timers.legend().setVisible(True)
        elif graph_type == "Pie":
            self.chart_timers.setTitle('Pie Chart')
            self.series = QPieSeries()

            for x, y in zip(xAxis, yAxis):
                slice = self.series.append(f'{x}', y)
                slice.setLabel(f"{x}:{y}")
                font = QFont()
                font.setPointSize(8)  # set the font size to 8
                slice.setLabelFont(font)
            self.series.setLabelsVisible(True)
            self.chart_timers.addSeries(self.series)
            self.chart_timers.createDefaultAxes()
            self.chart_timers.legend().hide()

        self.chart_timers.setAnimationOptions(QChart.SeriesAnimations)
        self.chart_timers.legend().setAlignment(Qt.AlignBottom)

    def on_click_reset_timer(self):
        self.chart_timers.removeAllSeries()
        self.chart_timers.hide()

    def on_click_table_timer(self):
        xAxis = [(key[24::]) for key, value in dictionary.items() if
                 ('TIMER' in key)]
        yAxis = [int(value) for key, value in dictionary.items() if
                 ('TIMER' in key)]
        self.table_timers.setColumnCount(2)
        self.table_timers.setRowCount(len(xAxis))
        self.table_timers.setHorizontalHeaderLabels(["device", "number"])
        for i, (x,y) in enumerate(zip(xAxis, yAxis)):
            self.table_timers.setItem(i, 0, QTableWidgetItem(str(x)))
            self.table_timers.setItem(i, 1, QTableWidgetItem(str(y)))

    def add_counters_data(self):
        groupBox = QGroupBox("Counters")
        groupBox.setFont(QFont('Times', 8))
        self.button_dispaly_counters = QPushButton('Display graph', self)
        self.button_dispaly_counters.setToolTip('Display')
        self.button_dispaly_counters.clicked.connect(self.on_click_display_counter)

        self.button_reset = QPushButton('Reset', self)
        self.button_reset.setToolTip('Reset')
        self.button_reset.clicked.connect(self.on_click_reset_counter)

        graph_type_text_counters = QLabel(self)
        graph_type_text_counters .setText('Graph type:')
        self.graph_type_combo_counters = QComboBox(self)
        self.graph_type_combo_counters.addItems(['Pie', 'Bar'])

        self.button_table_counters = QPushButton('Display table', self)
        self.button_table_counters.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.button_table_counters.setToolTip('Table')
        self.button_table_counters.clicked.connect(self.on_click_table_counter)

        self.chart_counters = QChart()
        self.chart_counters.setTheme(QChart.ChartThemeQt)
        self.chart_counters.setBackgroundBrush(QBrush(QColor("transparent")))

        self.chart_view_counters = QChartView(self.chart_counters)
        self.chart_view_counters.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.chart_view_counters.setMinimumSize(500, 500)

        self.table_counters = QTableWidget()
        self.table_counters.setStyleSheet("background-color: transparent; border-style: none;")
        self.table_counters.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.table_counters.setMinimumSize(200, 200)

        vbox = QVBoxLayout()
        vbox.addWidget(graph_type_text_counters )
        vbox.addWidget(self.graph_type_combo_counters)
        vbox.addWidget(self.button_dispaly_counters)
        vbox.addWidget(self.button_reset)
        vbox.addWidget(self.button_table_counters)
        vbox.addWidget(self.chart_view_counters)
        vbox.addWidget(self.table_counters)

        vbox.addStretch(1)
        groupBox.setLayout(vbox)
        return groupBox

    def on_click_display_counter(self):
        graph_type_counters = self.graph_type_combo_counters.currentText()
        self.chart_counters.removeAllSeries()
        self.chart_counters.show()
        xAxis = [(key[24::]).split("_")[0] for key, value in dictionary.items() if
                         ('DEVICES_COUNTER' in key and key!='RS2_AUS_CONNECTED_DEVICES_COUNTER')]
        yAxis = [int(value) for key, value in dictionary.items() if
                         ('DEVICES_COUNTER' in key and key != 'RS2_AUS_CONNECTED_DEVICES_COUNTER')]

        if graph_type_counters == "Bar":
            self.chart_counters.setTitle('Bar Graph')
            self.series = QBarSeries()
            self.set = QBarSet('')
            self.set.append(yAxis)
            self.series.append(self.set)
            self.chart_counters.addSeries(self.series)
            axis_x = QBarCategoryAxis()
            axis_x.append(xAxis)
            self.chart_counters.createDefaultAxes()
            self.chart_counters.axisY(self.series).setTitleText("Value")
            self.chart_counters.setAxisX(axis_x, self.series)
            self.chart_counters.axisX(self.series).setTitleText("Device")
            self.chart_counters.legend().hide()
        elif graph_type_counters == "Pie":
            self.chart_counters.setTitle('Pie Chart')
            self.series = QPieSeries()
            for x, y in zip(xAxis, yAxis):
                slice = self.series.append(f'{x}', y)
                slice.setLabel(f'{y}')
            self.series.setLabelsVisible(True)
            self.series.setLabelsPosition(QPieSlice.LabelOutside)
            self.chart_counters.addSeries(self.series)
            self.chart_counters.legend().setVisible(True)
            self.chart_counters.createDefaultAxes()
            markers = self.chart_counters.legend().markers(self.series)
            for i, x in enumerate(xAxis):
                markers[i].setLabel(f'{x}')


        self.chart_counters.setAnimationOptions(QChart.SeriesAnimations)
        self.chart_counters.legend().setAlignment(Qt.AlignBottom)

    def on_click_reset_counter(self):
        self.chart_counters.removeAllSeries()
        self.chart_counters.hide()

    def on_click_table_counter(self):
        xAxis = [(key[24::]).split("_")[0] for key, value in dictionary.items() if
                 ('DEVICES_COUNTER' in key and key != 'RS2_AUS_CONNECTED_DEVICES_COUNTER')]
        yAxis = [int(value) for key, value in dictionary.items() if
                 ('DEVICES_COUNTER' in key and key != 'RS2_AUS_CONNECTED_DEVICES_COUNTER')]
        self.table_counters.setColumnCount(2)
        self.table_counters.setRowCount(len(xAxis))
        self.table_counters.setHorizontalHeaderLabels(["device", "number"])
        for i, (x,y) in enumerate(zip(xAxis, yAxis)):
            self.table_counters.setItem(i, 0, QTableWidgetItem(str(x)))
            self.table_counters.setItem(i, 1, QTableWidgetItem(str(y)))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    screen = AUS()
    screen.show()
    sys.exit(app.exec_())