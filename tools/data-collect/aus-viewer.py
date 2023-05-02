import sys
import os
from PyQt5.QtWidgets import *
from PyQt5.QtChart import *
from PyQt5.QtGui import *
from PyQt5.QtCore import QDateTime, QDate, QTime, Qt

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
        'librealsense_version': '2.5.4',
        'data collection start time' : 'Mon Oct  2 00:59:08 2017',
        'data collection end time' : 'Mon Oct  2 00:59:08 2017',
        'ppf_used_per_device': { "Intel RealSense D435I" : [
                                    "Decimation Filter",
                                    "Depth to Disparity",
                                    "Disparity to Depth",
                                    "Spatial Filter",
                                    "Temporal Filter",
                                    "Threshold Filter"
                                    ],
                                "Intel RealSense D457"  : [
                                    "Disparity to Depth",
                                    "Spatial Filter",
                                    "Temporal Filter",
                                    "Threshold Filter"
                                    ]

        },
        'sensors_used_per_device': { 'Color': {
                                        "Intel RealSense D435I":[['11:34:42','11:37:43'], ['11:39:42','11:41:24']]
                                            ,"Intel RealSense D457":[['11:34:42','11:37:43'], ['11:39:42','11:41:24']],
                                            "Intel RealSense D415":[['11:34:42','11:38:24'], ['11:39:42','11:41:24']]

        },
            'Accel': {
                "Intel RealSense D435I":[['11:34:42','11:38:24'], ['11:39:42','11:41:24'] ]
                , "Intel RealSense D457":[['11:34:42','11:37:43'], ['11:39:42','11:41:24']],
            },
            'Gyro' : {
                "Intel RealSense D435I":[['11:34:42','11:38:43']],
                    "Intel RealSense D415":[['11:34:42','11:38:24'], ['11:39:42','11:41:24']]

            },
            'Depth': {
                "Intel RealSense D435I":[['11:34:42','11:34:51']],
                "Intel RealSense D415":[['11:34:42','11:38:24'], ['11:39:42','11:41:24']]
            }
        }

    }

ppf_list = ["Decimation Filter", "Depth to Disparity", "Disparity to Depth", "HDR Merge Filter", "Hole Filling Filter", "Filter By Sequence id", "Spatial Filter", "Temporal Filter", "Threshold Filter"]


class AUS(QMainWindow):
    def __init__(self):
        super().__init__()
        self.title = 'RealSense AUS Viewer'
        logo_path = os.path.join(os.getcwd(), 'icon.ico')
        logo = QIcon(logo_path)
        self.setWindowIcon(logo)
        self.left = 10
        self.top = 100
        self.width = 540
        self.height = 550
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        self.initUI()

    def initUI(self):
        grid = QGridLayout(self.centralWidget())
        grid.addWidget(self.static_data_and_devices(), 0, 0)
        grid.addWidget(self.add_timers_data(), 0, 2)
        grid.addWidget(self.ppf_per_device(), 0, 1)

        self.setLayout(grid)
        self.setWindowTitle(self.title)
        self.setGeometry(self.left, self.top, self.width, self.height)
        self.show()

    def static_data_and_devices(self):
        groupBox = QGroupBox("Static data")
        groupBox.setFont(QFont('Times', 8))
        self.add_static_data_labels()

        self.chart_counters = QChart()
        self.chart_view_counters = QChartView(self.chart_counters)
        self.chart_view_counters.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.chart_view_counters.setMinimumSize(500, 500)
        self.chart_counters.setTheme(QChart.ChartThemeQt)
        self.chart_counters.setBackgroundBrush(QBrush(QColor("transparent")))
        self.add_pie_chart_data()

        vbox = QVBoxLayout()
        vbox.addWidget(self.os_name_text)
        vbox.addWidget(self.os_name)
        vbox.addWidget(self.platform_name_text)
        vbox.addWidget(self.platform_name)
        vbox.addWidget(self.librealsense_version_text)
        vbox.addWidget(self.librealsense_version)
        vbox.addWidget(self.data_collection_start_time_text)
        vbox.addWidget(self.data_collection_start_time)
        vbox.addWidget(self.data_collection_end_time_text)
        vbox.addWidget(self.data_collection_end_time)
        vbox.addWidget(self.chart_view_counters)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)
        return groupBox
    def add_static_data_labels(self):
        self.os_name_text = QLabel(self)
        self.os_name_text.setFont(QFont('Times', 8))
        self.os_name_text.setText('OS:')
        self.os_name = QLineEdit(self)
        self.os_name.setFont(QFont('Times', 8))
        self.os_name.setText(dictionary['os_name'])
        self.os_name.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.os_name.setReadOnly(True)
        self.platform_name_text = QLabel(self)
        self.platform_name_text.setText('Platform:')
        self.platform_name_text.setFont(QFont('Times', 8))
        self.platform_name = QLineEdit(self)
        self.platform_name.setFont(QFont('Times', 8))
        self.platform_name.setText(dictionary['platform_name'])
        self.platform_name.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.platform_name.setReadOnly(True)
        self.librealsense_version_text = QLabel(self)
        self.librealsense_version_text.setText('Librealsense version:')
        self.librealsense_version_text.setFont(QFont('Times', 8))
        self.librealsense_version = QLineEdit(self)
        self.librealsense_version.setFont(QFont('Times', 8))
        self.librealsense_version.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.librealsense_version.setText(dictionary['librealsense_version'])
        self.librealsense_version.setReadOnly(True)
        self.data_collection_start_time_text = QLabel(self)
        self.data_collection_start_time_text.setFont(QFont('Times', 8))
        self.data_collection_start_time_text.setText('Data collection start time:')
        self.data_collection_start_time = QLineEdit(self)
        self.data_collection_start_time.setFont(QFont('Times', 8))
        self.data_collection_start_time.setText(dictionary['data collection start time'])
        self.data_collection_start_time.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.data_collection_start_time.setReadOnly(True)
        self.data_collection_end_time_text = QLabel(self)
        self.data_collection_end_time_text.setFont(QFont('Times', 8))
        self.data_collection_end_time_text.setText('Data collection end time:')
        self.data_collection_end_time = QLineEdit(self)
        self.data_collection_end_time.setFont(QFont('Times', 8))
        self.data_collection_end_time.setText(dictionary['data collection end time'])
        self.data_collection_end_time.setStyleSheet("QLineEdit""{""background : lightgrey;""}")
        self.data_collection_end_time.setReadOnly(True)
    def add_pie_chart_data(self):
        self.chart_counters.setTitle('Devices')
        self.series = QPieSeries()
        self.chart_counters.show()
        xAxis = [(key[24::]).split("_")[0] for key, value in dictionary.items() if
                 ('DEVICES_COUNTER' in key and key != 'RS2_AUS_CONNECTED_DEVICES_COUNTER')]
        yAxis = [int(value) for key, value in dictionary.items() if
                 ('DEVICES_COUNTER' in key and key != 'RS2_AUS_CONNECTED_DEVICES_COUNTER')]
        colors_pie_chart = [
            "#0072B2",  # blue
            "#E69F00",  # orange
            "#56B4E9",  # light blue
            "#009E73",  # green
            "#F0E442",  # yellow
            "#D55E00",  # red-orange
            "#CC79A7",  # pink
            "#999999",  # gray
            "#FFC20A",  # yellow-orange
            "#007BA7"  # dark-blue
        ]
        for x, y, color in zip(xAxis, yAxis, colors_pie_chart):
            slice_pie = self.series.append(f'{x}', y)
            slice_pie.setLabel(f'{x}:{y}')
            slice_pie.setBrush(QColor(color))

        self.series.setLabelsVisible(True)
        self.series.setLabelsPosition(QPieSlice.LabelInsideHorizontal)
        self.chart_counters.addSeries(self.series)
        self.chart_counters.legend().hide()
        self.chart_counters.createDefaultAxes()
        self.chart_counters.setAnimationOptions(QChart.SeriesAnimations)
        self.chart_counters.legend().setAlignment(Qt.AlignBottom)
    def add_timers_data(self):
        groupBox = QGroupBox("Sensors")
        groupBox.setFont(QFont('Times', 8))
        vbox = QVBoxLayout()

        for sensor_type, sensor_info in dictionary['sensors_used_per_device'].items():
            chart = QChart()
            chart.setTitle(f"{sensor_type} ")
            x_axis = QDateTimeAxis()
            x_axis.setFormat('hh:mm:ss')
            x_axis.setTitleText('Time')
            chart.addAxis(x_axis, Qt.AlignBottom)
            chart.legend().setAlignment(Qt.AlignBottom)
            for index, (device, data) in enumerate(sensor_info.items()):
                series = QLineSeries()
                series.setPointLabelsFormat("@xPoint")
                series.setName(f"{device[16::]}")
                previous_end_time = None

                for time_range in data:
                    start_time, end_time = time_range
                    start_time_obj = QTime.fromString(start_time, 'hh:mm:ss')
                    end_time_obj = QTime.fromString(end_time, 'hh:mm:ss')
                    start_datetime = QDateTime(QDate.currentDate(), start_time_obj)
                    end_datetime = QDateTime(QDate.currentDate(), end_time_obj)

                    if previous_end_time is not None:
                        previous_end_datetime = QDateTime(QDate.currentDate(), previous_end_time)
                        if start_datetime > previous_end_datetime:
                            gap = previous_end_datetime.secsTo(start_datetime)
                            for j in range(gap + 1):
                                timestamp = previous_end_datetime.addSecs(j).toMSecsSinceEpoch()
                                value_off = 0.5*(0+(index*2))
                                series.append(timestamp, value_off)
                    delta = start_datetime.secsTo(end_datetime)
                    for k in range(delta + 1):
                        timestamp = start_datetime.addSecs(k).toMSecsSinceEpoch()
                        value_on = 0.5*(1+(index*2))
                        series.append(timestamp, value_on)
                    previous_end_time = end_time_obj

                chart.addSeries(series)
                y_axis = QValueAxis()
                y_axis.setRange(0, len(sensor_info.items()))
                y_axis.setLabelsVisible(False)
                if index == 0:
                    y_axis.setTitleText('On/Off')
                else:
                    y_axis.setLineVisible(False)
                chart.addAxis(y_axis, Qt.AlignLeft if index == 0 else Qt.AlignRight)
                series.attachAxis(x_axis)
                series.attachAxis(y_axis)

            chart_view = QChartView(chart)
            chart_view.setMinimumSize(150, 250)
            chart_view.setRenderHint(QPainter.Antialiasing)
            chart_view.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            vbox.addWidget(chart_view)

        vbox.addStretch(1)
        groupBox.setLayout(vbox)
        return groupBox

    def ppf_per_device(self):
        self.ppf_data = dictionary['ppf_used_per_device']
        device_text = QLabel(self)
        device_text.setText('Device:')
        self.device_combo = QComboBox(self)
        self.device_combo.addItem("All devices")
        self.device_combo.addItems([key[16::] for key in self.ppf_data])
        groupBox = QGroupBox("Post processing filters")
        groupBox.setFont(QFont('Times', 8))

        self.button_dispaly_device_option = QPushButton('Display ppf list', self)
        self.button_dispaly_device_option.setToolTip('Display list')
        self.button_dispaly_device_option.clicked.connect(self.on_click_display_ppf_list)

        self.list_widget = QListWidget()
        self.list_widget.setStyleSheet("background-color: transparent; border-style: none;")
        self.table_ppf = QTableWidget()
        self.table_ppf.setStyleSheet("background-color: transparent; border-style: none;")
        self.table_ppf.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.table_ppf.setMinimumSize(500,500)

        self.vbox_ppf = QVBoxLayout()
        self.vbox_ppf.addWidget(device_text)
        self.vbox_ppf.addWidget(self.device_combo)
        self.vbox_ppf.addWidget(self.button_dispaly_device_option)
        self.vbox_ppf.addWidget(self.table_ppf)
        self.vbox_ppf.addWidget(self.list_widget)

        self.vbox_ppf.addStretch(1)
        groupBox.setLayout(self.vbox_ppf)
        return groupBox
    def on_click_display_ppf_list(self):
        self.list_widget.clear()
        device_combo_chosen = self.device_combo.currentText()
        if device_combo_chosen == "All devices":
            xAxis = [key[16::] for key in self.ppf_data]
            yAxis = ppf_list
            self.table_ppf.setColumnCount(len(xAxis))
            self.table_ppf.setRowCount(len(yAxis))
            self.table_ppf.setHorizontalHeaderLabels(xAxis)
            self.table_ppf.setVerticalHeaderLabels(yAxis)
            for i, x in enumerate(xAxis):
                for j, y in enumerate(yAxis):
                    if y in self.ppf_data["Intel RealSense "+x]:
                        item = QTableWidgetItem("X")
                        item.setTextAlignment(Qt.AlignCenter)
                        self.table_ppf.setItem(i, j, item)
        else:
            ppf_list_of_chosen_device = self.ppf_data["Intel RealSense "+device_combo_chosen]
            self.list_widget.addItems(ppf_list_of_chosen_device)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    screen = AUS()
    screen.show()
    sys.exit(app.exec_())
