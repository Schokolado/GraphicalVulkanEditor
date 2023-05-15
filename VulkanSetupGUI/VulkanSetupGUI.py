from PyQt5 import uic, sip
from PyQt5.QtCore import Qt, QByteArray
from PyQt5.QtGui import QPixmap, QImage, QOpenGLFramebufferObject, QPainter, QSurfaceFormat, \
    QOpenGLFramebufferObjectFormat
from PyQt5.QtOpenGL import QGLWidget
from pywavefront import Wavefront
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
    else:
        print(f"Provided input was [{input}]. Unnecessary conversion call.")
        return input


class OpenGLWidget(QOpenGLWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.obj_file_path = "C:/Users/Avoccardo/source/repos/VulkanSetup/models/viking_room_blender.obj"  # Replace with the path to your OBJ file
        self.obj_mesh = None

    def initializeGL(self):
        glClearColor(0.0, 0.0, 0.0, 1.0)

        # Load the OBJ file
        self.obj_mesh = Wavefront(self.obj_file_path, collect_faces=True)

        # Enable lighting
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)

        # Set light position
        light_position = [1.0, 1.0, 1.0, 0.0]  # Directional light from (1, 1, 1)
        glLightfv(GL_LIGHT0, GL_POSITION, light_position)

        # Set light properties
        ambient_color = [0.2, 0.2, 0.2, 1.0]
        diffuse_color = [0.5, 0.5, 0.5, 1.0]
        specular_color = [1.0, 1.0, 1.0, 1.0]
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color)
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color)
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color)

    def resizeGL(self, width, height):
        glViewport(0, 0, width, height)

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glEnable(GL_DEPTH_TEST)

        # Enable lighting
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)

        ambient_color = [0.4, 0.4, 0.4, 1.0]  # Adjust the values as needed
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color)
        diffuse_color = [1.2, 1.2, 1.2, 1.0]  # Adjust the values as needed
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color)

        # Set light position
        light_position = [1.0, 1.0, 1.0, 1.0]  # Adjust the position values as needed
        glLightfv(GL_LIGHT0, GL_POSITION, light_position)

        glBegin(GL_TRIANGLES)
        glColor3f(1.0, 0.0, 0.0)  # Red color
        glVertex2f(-0.6, -0.4)
        glColor3f(0.0, 1.0, 0.0)  # Green color
        glVertex2f(0.6, -0.4)
        glColor3f(0.0, 0.0, 1.0)  # Blue color
        glVertex2f(0.0, 0.6)

        # Set material properties
        ambient_material = [0.2, 0.2, 0.2, 1.0]
        diffuse_material = [1.0, 1.0, 1.0, 1.0]
        specular_material = [1.0, 1.0, 1.0, 1.0]
        shininess_material = 32.0
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient_material)
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse_material)
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular_material)
        glMaterialf(GL_FRONT, GL_SHININESS, shininess_material)

        if self.obj_mesh:
            for mesh in self.obj_mesh.mesh_list:

                for face in mesh.faces:
                    for vertex_i in face:
                        vertex = self.obj_mesh.vertices[vertex_i]
                        glVertex3fv(vertex)
                glColor3f(1.0, 1.0, 1.0)

        glEnd()


class GraphicsPipelineView(QDialog):
    def __init__(self):
        super(GraphicsPipelineView, self).__init__()
        uic.loadUi("GraphicsPipelineView.ui", self)


class VulkanSetupGUI(QMainWindow):
    def __init__(self):
        super(VulkanSetupGUI, self).__init__()
        uic.loadUi("VulkanSetupGUI.ui", self)
        # uic.loadUi("pipelineTestView.ui", self)

        self.connectActions()
        self.setupModelPreview()
        self.loadTexturePreview()

    def loadTexturePreview(self):
        self.texturePreviewImage.setPixmap(QPixmap(self.textureFileInput.text()))
        pass

    def setupModelPreview(self):

        modelPreviewOpenGLWidget = OpenGLWidget()
        modelPreviewOpenGLWidget.setMinimumSize(255, 255)
        modelPreviewOpenGLWidget.setMaximumSize(255, 255)
        self.verticalLayoutModelPreview.addWidget(modelPreviewOpenGLWidget)

    def connectActions(self):
        # Menu Bar and Bottom
        self.actionExport_to_Vulkan.triggered.connect(self.setOutput)
        self.generateVulkanCodeButton.clicked.connect(self.setOutput)
        self.actionSave_to_File.triggered.connect(self.writeXml)
        self.actionLoad_from_File.triggered.connect(self.readXml)
        self.actionLoad_from_File.triggered.connect(self.loadTexturePreview)

        # Instance

        # Physical Device

        # Logical Device
        self.addExtensionButton.clicked.connect(self.showAddExtensionInput)
        self.removeExtensionButton.clicked.connect(self.showRemoveExtension)

        # Swapchain

        # Model
        self.modelFileToolButton.clicked.connect(lambda: self.setFilePath("modelFileToolButton"))
        self.textureFileToolButton.clicked.connect(lambda: self.setFilePath("textureFileToolButton"))

        # GraphicsPipeline
        self.addPipelineButton.clicked.connect(self.showAddPipelineInput)
        # self.editPipelineButton.clicked.connect(self.showEditPipelineInput)
        self.deletePipelineButton.clicked.connect(self.showRemovePipeline)
        # self.pipelinePreviewButton.clicked.connect(self.showPreview)

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
        print("imageHeightInput: [{}]".format(
            self.imageHeightInput.text()))
        print("imageWidthInput: [{}]".format(
            self.imageWidthInput.text()))
        print("lockWindowSizeCheckBox: [{}]".format(
            convertToVulkanNaming(self.lockWindowSizeCheckBox.isChecked())))
        print("clearColorRInput: [{}]".format(
            self.clearColorRInput.text()))
        print("clearColorGInput: [{}]".format(
            self.clearColorGInput.text()))
        print("clearColorBInput: [{}]".format(
            self.clearColorBInput.text()))
        print("clearColorAInput: [{}]".format(
            self.clearColorAInput.text()))
        print("framesInFlightInput: [{}]".format(
            self.framesInFlightInput.text()))
        print("saveEnergyForMobileCheckBox: [{}]".format(
            convertToVulkanNaming(self.saveEnergyForMobileCheckBox.isChecked())))
        print("imageUsageInput: [{}]".format(
            self.imageUsageInput.currentText()))
        print("presentationModeInput: [{}]".format(
            self.presentationModeInput.currentText()))
        print("imageFormatInput: [{}]".format(
            self.imageFormatInput.currentText()))
        print("imageColorSpaceInput: [{}]".format(
            self.imageColorSpaceInput.currentText()))

        # Model
        print("modelFileInput: [{}]".format(
            self.modelFileInput.text()))
        print("textureFileInput: [{}]".format(
            self.textureFileInput.text()))

        # GraphicsPipeline

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

    def showAddPipelineInput(self):
        def showPipelineAlreadyPresent(extension: str):
            d = QDialog(self)
            d.setWindowTitle("")
            d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
            layout = QVBoxLayout()
            label = QLabel(f"Pipeline already present:\n{extension}")
            layout.addWidget(label)
            ok_button = QPushButton("OK")
            ok_button.clicked.connect(d.accept)
            layout.addWidget(ok_button)
            d.setLayout(layout)
            d.show()

        def addPipeline(pipeline: str):
            if not pipeline:
                self.showNotAllowedInput("empty line")
                return

            if self.addUniqueItem(self.graphicsPipelinesList, pipeline):
                print(f"Pipeline Added: {pipeline}")
            else:
                showPipelineAlreadyPresent(pipeline)

        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Add new Pipeline:\n")
        layout.addWidget(label)
        lineEdit = QLineEdit()
        layout.addWidget(lineEdit)
        ok_button = QPushButton("Add")
        ok_button.clicked.connect(lambda: addPipeline(lineEdit.text()))
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def showRemovePipeline(self):

        def removePipelines(pipelines):
            listName = "removed pipelines"
            items = [item.text() for item in pipelines]

            for pipeline in items:
                self.removeItem(self.graphicsPipelinesList, pipeline)
            print(f"{listName}: {items}")

        # nothing to do if nothing selected
        if not self.graphicsPipelinesList.selectedItems():
            return

        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        pipelinesText = "\n".join(self.getListContents(self.graphicsPipelinesList))
        label = QLabel(f"Remove Pipeline?:\n{pipelinesText}")
        layout.addWidget(label)
        ok_button = QPushButton("OK")
        ok_button.clicked.connect(lambda: removePipelines(self.graphicsPipelinesList.selectedItems()))
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)
        d.setLayout(layout)
        d.show()

    def fileOpenDialog(self):
        fileName = QFileDialog.getOpenFileName(self, 'Open file',
                                               'c:\\', "Model files (*.jpg)")
        # self.modelPreviewOpenGLWidget = glWidget(self)

    def setFilePath(self, fileInput: str):
        def setInputFilterBasedOnOrigin(fileInput):
            filterMap = {
                "modelFileToolButton": "Model files (*.obj)",
                "textureFileToolButton": "Texture image files (*.jpg, *.png)",
                "vertexShaderFileToolButton": "Vertex shader files (*.vert)",
                "fragmentShaderFileToolButton": "fragment shader files (*.frag)"
            }
            return filterMap.get(fileInput, "")

        def setParametersBasedOnOrigin(fileInput, fileName):
            inputMap = {
                "modelFileToolButton": self.modelFileInput,
                "textureFileToolButton": self.textureFileInput,
                "vertexShaderFileToolButton": referenceView.vertexShaderFileInput,
                "fragmentShaderFileToolButton": referenceView.fragmentShaderFileInput
            }
            if fileInput not in inputMap:
                return
            inputField = inputMap[fileInput]
            if inputField != "" and fileName[0] == "":
                return
            inputField.setText(fileName[0])

        inputFilter = setInputFilterBasedOnOrigin(fileInput)
        initialDirectory = 'c:\\'
        fileName = QFileDialog.getOpenFileName(self, 'Open file', initialDirectory, inputFilter)
        referenceView = GraphicsPipelineView()
        setParametersBasedOnOrigin(fileInput, fileName)
        self.loadTexturePreview()

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

        # Swapchain
        swapchain = ET.SubElement(root, 'swapchain')

        imageDimensions = ET.SubElement(swapchain, 'imageDimensions', name='imageDimensions')
        ET.SubElement(imageDimensions, 'imageHeightInput', name='imageHeightInput').text = self.imageHeightInput.text()
        ET.SubElement(imageDimensions, 'imageWidthInput', name='imageWidthInput').text = self.imageWidthInput.text()

        ET.SubElement(swapchain, 'lockWindowSizeCheckBox',
                      name='lockWindowSizeCheckBox').text = convertToVulkanNaming(
            self.lockWindowSizeCheckBox.isChecked())

        imageClearColor = ET.SubElement(swapchain, 'imageClearColor', name='imageClearColor')
        ET.SubElement(imageClearColor, 'clearColorRInput', name='clearColorRInput').text = self.clearColorRInput.text()
        ET.SubElement(imageClearColor, 'clearColorGInput', name='clearColorGInput').text = self.clearColorGInput.text()
        ET.SubElement(imageClearColor, 'clearColorBInput', name='clearColorBInput').text = self.clearColorBInput.text()
        ET.SubElement(imageClearColor, 'clearColorAInput', name='clearColorAInput').text = self.clearColorAInput.text()

        ET.SubElement(swapchain, 'framesInFlightInput',
                      name='framesInFlightInput').text = self.framesInFlightInput.text()
        ET.SubElement(swapchain, 'saveEnergyForMobileCheckBox',
                      name='saveEnergyForMobileCheckBox').text = convertToVulkanNaming(
            self.saveEnergyForMobileCheckBox.isChecked())
        ET.SubElement(swapchain, 'imageUsageInput',
                      name='imageUsageInput').text = self.imageUsageInput.currentText()
        ET.SubElement(swapchain, 'presentationModeInput',
                      name='presentationModeInput').text = self.presentationModeInput.currentText()
        ET.SubElement(swapchain, 'imageFormatInput',
                      name='imageFormatInput').text = self.imageFormatInput.currentText()
        ET.SubElement(swapchain, 'imageColorSpaceInput',
                      name='imageColorSpaceInput').text = self.imageColorSpaceInput.currentText()

        # Texture and Model
        model = ET.SubElement(root, 'model')
        ET.SubElement(model, 'modelFileInput', name='modelFileInput').text = self.modelFileInput.text()
        ET.SubElement(model, 'textureFileInput', name='textureFileInput').text = self.textureFileInput.text()

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
                if elem.text == "VK_FALSE":
                    self.showValidationLayerDebugInfoCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
                    self.showValidationLayerDebugInfoCheckBox.setChecked(True)
            if elem.tag == "runOnMacosCheckBox":
                if elem.text == "VK_FALSE":
                    self.runOnMacosCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
                    self.runOnMacosCheckBox.setChecked(True)

            ### Physical Device
            if elem.tag == "chooseGPUOnStartupCheckBox":
                if elem.text == "VK_FALSE":
                    self.chooseGPUOnStartupCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
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

            ### Swapchain
            if elem.tag == "imageHeightInput":
                self.imageHeightInput.setValue(int(elem.text))
            if elem.tag == "imageWidthInput":
                self.imageWidthInput.setValue(int(elem.text))
            if elem.tag == "lockWindowSizeCheckBox":
                if elem.text == "VK_FALSE":
                    self.lockWindowSizeCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
                    self.lockWindowSizeCheckBox.setChecked(True)
            if elem.tag == "clearColorRInput":
                self.clearColorRInput.setValue(float(elem.text.replace(',', '.')))
            if elem.tag == "clearColorGInput":
                self.clearColorGInput.setValue(float(elem.text.replace(',', '.')))
            if elem.tag == "clearColorBInput":
                self.clearColorBInput.setValue(float(elem.text.replace(',', '.')))
            if elem.tag == "clearColorAInput":
                self.clearColorAInput.setValue(float(elem.text.replace(',', '.')))
            if elem.tag == "framesInFlightInput":
                self.framesInFlightInput.setValue(int(elem.text))
            if elem.tag == "saveEnergyForMobileCheckBox":
                if elem.text == "VK_FALSE":
                    self.saveEnergyForMobileCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
                    self.saveEnergyForMobileCheckBox.setChecked(True)
            if elem.tag == "imageUsageInput":
                index = self.imageUsageInput.findText(elem.text)
                self.imageUsageInput.setCurrentIndex(index)
            if elem.tag == "presentationModeInput":
                index = self.presentationModeInput.findText(elem.text)
                self.presentationModeInput.setCurrentIndex(index)
            if elem.tag == "imageFormatInput":
                index = self.imageFormatInput.findText(elem.text)
                self.imageFormatInput.setCurrentIndex(index)
            if elem.tag == "imageColorSpaceInput":
                index = self.imageColorSpaceInput.findText(elem.text)
                self.imageColorSpaceInput.setCurrentIndex(index)

            ### Model
            if elem.tag == "modelFileInput":
                self.modelFileInput.setText(elem.text)
            if elem.tag == "textureFileInput":
                self.textureFileInput.setText(elem.text)

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
