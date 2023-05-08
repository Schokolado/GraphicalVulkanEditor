from PyQt5 import uic
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPixmap
from PyQt5.QtOpenGL import QGLWidget
from PyQt5.QtWidgets import *
from OpenGL.GL import *
from OpenGL.GLU import *
import xml.etree.ElementTree as ET

# Form, Window = uic.loadUiType("VulkanSetupGUI.ui")

# app = QApplication([])
# window = Window()
# form = Form()
# form.setupUi(window)
# window.show()
# app.exec()

class VulkanSetupGUI:
    pass


DEFAULT_DEVICE_EXTENSIONS = ["VK_KHR_SWAPCHAIN_EXTENSION_NAME"]


def convertToVulkanNaming(input: str):
    if type(input) == bool:
        if input:
            return "VK_TRUE"
        if not input:
            return "VK_FALSE"


class VulkanSetupGUI(QMainWindow):
    def __init__(self):
        super(VulkanSetupGUI, self).__init__()
        uic.loadUi("VulkanSetupGUI.ui", self)
        # uic.loadUi("pipelineTestView.ui", self)

        self.connectActions()

    def connectActions(self):
        # Menu Bar and Bottom
        self.actionExport_to_Vulkan.triggered.connect(self.setOutput)
        self.generateVulkanCodeButton.clicked.connect(self.setOutput)
        self.actionSave_to_File.triggered.connect(self.writeXml)
        self.actionLoad_from_File.triggered.connect(self.readXml)

        # Instance

        # Physical Device

        # Logical Device
        self.addExtensionButton.clicked.connect(self.showAddExtensionInput)
        self.removeExtensionButton.clicked.connect(self.showRemoveExtension)

        # Swapchain

        # Model
        self.modelFileToolButton.clicked.connect(self.fileOpenDialog)

        #GraphicsPipeline

    ### Print and Output Section

    def setOutput(self):
        # Check if all input is set and checked
        if not self.checkInput():
            return

        self.printInputs()
    def printInputs(self):
        # Instance
        print("applicationNameInput: [{}]".format(
            self.applicationNameInput.text()))
        print("showValidationLayerDebugInfoCheckBox: [{}]".format(
            convertToVulkanNaming(self.showValidationLayerDebugInfoCheckBox.isChecked())))
        print("runOnMacosCheckBox: [{}]".format(
            convertToVulkanNaming(self.runOnMacosCheckBox.isChecked())))

        # Physical Device
        print("chooseGPUOnStartupCheckBox: [{}]".format(
            convertToVulkanNaming(self.chooseGPUOnStartupCheckBox.isChecked())))

        # Logical Device
        print("deviceExtensionsList: [{}]".format(
            self.getListContents(self.deviceExtensionsList)))

        # Swapchain

        # Model

        #GraphicsPipeline

        print()

    ### Show Views Section

    def showRemoveExtension(self):
        def checkValidRemoval(item):
            return item not in DEFAULT_DEVICE_EXTENSIONS

        def removeExtensions(extensions):
            listName = "removed extensions"
            items = [item.text() for item in extensions if checkValidRemoval(item.text())]

            for extension in items:
                self.removeItem(self.deviceExtensionsList, extension)
            print(f"{listName}: {items}")

        # nothing to do if nothing selected
        if not self.deviceExtensionsList.selectedItems():
            return

        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        extensionsText = "\n".join(self.getListContents(self.deviceExtensionsList))
        label = QLabel(f"Remove Extensions?:\n{extensionsText}")
        layout.addWidget(label)
        ok_button = QPushButton("OK")
        ok_button.clicked.connect(lambda: removeExtensions(self.deviceExtensionsList.selectedItems()))
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def showAddExtensionInput(self):
        def showExtensionAlreadyPresent(extension: str):
            d = QDialog(self)
            d.setWindowTitle("")
            d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
            layout = QVBoxLayout()
            label = QLabel(f"Extension already present:\n{extension}")
            layout.addWidget(label)
            ok_button = QPushButton("OK")
            ok_button.clicked.connect(d.accept)
            layout.addWidget(ok_button)
            d.setLayout(layout)
            d.show()

        def addExtension(extension: str):
            if not extension:
                self.showNotAllowedInput("empty line")
                return

            if self.addUniqueItem(self.deviceExtensionsList, extension):
                print(f"Extension Added: {extension}")
            else:
                showExtensionAlreadyPresent(extension)

        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Add new Extension:\n")
        layout.addWidget(label)
        lineEdit = QLineEdit()
        layout.addWidget(lineEdit)
        ok_button = QPushButton("Add")
        ok_button.clicked.connect(lambda: addExtension(lineEdit.text()))
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def showNotAllowedInput(self, notAllowed):
        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Input not allowed:\n\n\"{notAllowed}\"\n")
        layout.addWidget(label)
        ok_button = QPushButton("OK")
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def showMissingInput(self, missing):
        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Input was missing:\n\n{missing}\n")
        layout.addWidget(label)
        ok_button = QPushButton("OK")
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def showDefaultExtensionsUsed(self):
        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        extensionsText = "\n".join(DEFAULT_DEVICE_EXTENSIONS)
        label = QLabel(f"No Extensions selected. Using default Extensions:\n\n{extensionsText}\n"
                       f"\nPlease Generate Vulkan Code again.")
        layout.addWidget(label)
        ok_button = QPushButton("OK")
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def fileOpenDialog(self):
        fileName = QFileDialog.getOpenFileName(self, 'Open file',
                                               'c:\\', "Model files (*.jpg)")
        # self.modelPreviewOpenGLWidget = glWidget(self)



    ### Validation Section
    def checkInput(self):
        if len(self.applicationNameInput.text()) == 0:
            self.showMissingInput((self.applicationNameLabel.text()).replace(":", ""))
            return False

        if not self.checkDeviceExtensions():
            return False
        # Input checked, all OK
        return True

    def checkDeviceExtensions(self):
        # If no item is selected, set selection to required default extensions
        if len(self.deviceExtensionsList.selectedItems()) == 0:
            itemAdded = False
            self.showDefaultExtensionsUsed()
            for extension in DEFAULT_DEVICE_EXTENSIONS:
                if self.addUniqueItem(self.deviceExtensionsList, extension):
                    itemAdded = True

            self.selectMultipleItems(self.deviceExtensionsList, DEFAULT_DEVICE_EXTENSIONS)
            return itemAdded
        else:
            return True

    ### Items and List section

    def selectMultipleItems(self, listWidget, itemTexts):
        for i in range(listWidget.count()):
            item = listWidget.item(i)
            if item.text() in itemTexts:
                item.setSelected(True)

    def addUniqueItem(self, listWidget, item):
        for i in range(listWidget.count()):
            if listWidget.item(i).text() == item:
                return None
        listWidget.addItem(item)
        return listWidget.item(listWidget.count() - 1)

    def removeItem(self, listWidget, item):
        for i in range(listWidget.count()):
            if listWidget.item(i).text() == item:
                listWidget.takeItem(i)
                return True
        return False

    def getListContents(self, listWidget):
        selectedItems = listWidget.selectedItems()
        items = [item.text() for item in selectedItems]
        return items

    def findExtension(self, extension):
        for i in range(self.deviceExtensionsList.count()):
            item = self.deviceExtensionsList.item(i)
            if item.text() == extension:
                # Do something with the item
                return item
                print(f"Found item with text 'extension': {item.text()}")

    def writeXml(self):
        # create the file structure
        root = ET.Element('VulkanSetup')

        # Instance
        instance = ET.SubElement(root, 'instance')
        ET.SubElement(instance, 'applicationNameInput',
                      name='applicationNameInput').text = \
            self.applicationNameInput.text()
        ET.SubElement(instance, 'showValidationLayerDebugInfoCheckBox',
                      name='showValidationLayerDebugInfoCheckBox').text = convertToVulkanNaming(
            self.showValidationLayerDebugInfoCheckBox.isChecked())
        ET.SubElement(instance, 'runOnMacosCheckBox', name='runOnMacosCheckBox').text = convertToVulkanNaming(
            self.runOnMacosCheckBox.isChecked())

        # Physical Device
        physicalDevice = ET.SubElement(root, 'physicalDevice')
        ET.SubElement(physicalDevice, 'chooseGPUOnStartupCheckBox',
                      name='chooseGPUOnStartupCheckBox').text = convertToVulkanNaming(
            self.chooseGPUOnStartupCheckBox.isChecked())

        # Logical Device
        logicalDevice = ET.SubElement(root, 'logicalDevice')
        deviceExtensionsList = ET.SubElement(logicalDevice, 'deviceExtensionsList', name='deviceExtensionsList')
        for extension in self.getListContents(self.deviceExtensionsList):
            ET.SubElement(deviceExtensionsList, 'extension').text = extension

        # create a new XML file with the results
        mydata = ET.tostring(root)
        with open("VulkanSetup.xml", "wb") as myfile:
            myfile.write(mydata)

        print("XML File created with parameters:")
        self.printInputs()

    def readXml(self):
        tree = ET.parse('VulkanSetup.xml')
        root = tree.getroot()

        # recursively iterate over all elements and sub-elements
        def traverse(elem):
            print(f"\nElement: {elem.tag}, attributes: {elem.attrib}, text: {elem.text}")

            ### Instance
            if elem.tag == "applicationNameInput":
                self.applicationNameInput.setText(elem.text)
            if elem.tag == "showValidationLayerDebugInfoCheckBox":
                if(elem.text == "VK_FALSE"):
                    self.showValidationLayerDebugInfoCheckBox.setChecked(False)
                if(elem.text == "VK_TRUE"):
                    self.showValidationLayerDebugInfoCheckBox.setChecked(True)
            if elem.tag == "runOnMacosCheckBox":
                if(elem.text == "VK_FALSE"):
                    self.runOnMacosCheckBox.setChecked(False)
                if(elem.text == "VK_TRUE"):
                    self.runOnMacosCheckBox.setChecked(True)

            ### Physical Device
            if elem.tag == "chooseGPUOnStartupCheckBox":
                if(elem.text == "VK_FALSE"):
                    self.chooseGPUOnStartupCheckBox.setChecked(False)
                if(elem.text == "VK_TRUE"):
                    self.chooseGPUOnStartupCheckBox.setChecked(True)

            ### Logical Device
            if elem.tag == "extension":
                alreadyPresentItem = self.findExtension(elem.text)
                if alreadyPresentItem:
                    alreadyPresentItem.setSelected(True)
                else:
                    item = self.addUniqueItem(self.deviceExtensionsList, elem.text)
                    if item:
                        item.setSelected(True)
                        print(f"Extension Added: {elem.text}")

            for subelem in elem:
                traverse(subelem)

        traverse(root)

class glWidget(QOpenGLWidget):

    def __init__(self, parent):
        QOpenGLWidget.__init__(self, parent)

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        glTranslatef(-2.5, 0.5, -6.0)
        glColor3f(1.0, 1.5, 0.0)
        glPolygonMode(GL_FRONT, GL_FILL)
        glBegin(GL_TRIANGLES)
        glVertex3f(2.0, -1.2, 0.0)
        glVertex3f(2.6, 0.0, 0.0)
        glVertex3f(2.9, -1.2, 0.0)
        glEnd()
        glFlush()

    def initializeGL(self):
        glClearDepth(1.0)
        glDepthFunc(GL_LESS)
        glEnable(GL_DEPTH_TEST)
        glShadeModel(GL_SMOOTH)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(45.0, 1.33, 0.1, 100.0)
        glMatrixMode(GL_MODELVIEW)


def main():
    app = QApplication([])
    window = VulkanSetupGUI()
    window.show()
    app.exec_()


if __name__ == '__main__':
    main()
