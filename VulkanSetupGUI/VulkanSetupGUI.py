import ast
import os
import re

from PyQt5 import uic
from PyQt5.QtGui import QPixmap
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

def convertFromVulkanNaming(input):
    if input == "VK_TRUE":
        return True
    elif input == "VK_FALSE":
        return False
    else:
        return input

from PyQt5.QtCore import Qt


class OpenGLWidget(QOpenGLWidget):
    def __init__(self, obj_file_path, parent=None):
        super().__init__(parent)
        self.obj_file_path = obj_file_path
        self.obj_mesh = None

    def setOBJFilePath(self, path):
        self.obj_file_path = path

    def initializeGL(self):
        glClearColor(0.0, 0.0, 0.0, 1.0)
        self.obj_mesh = Wavefront(self.obj_file_path, collect_faces=True)
        self.setupLighting()

    def resizeGL(self, width, height):
        glViewport(0, 0, width, height)

    def updateGL(self):
        self.obj_mesh = Wavefront(self.obj_file_path, collect_faces=True)
        self.paintGL()

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glEnable(GL_DEPTH_TEST)

        glBegin(GL_TRIANGLES)
        if self.obj_mesh:
            for mesh in self.obj_mesh.mesh_list:
                for face in mesh.faces:
                    for vertex_i in face:
                        vertex = self.obj_mesh.vertices[vertex_i]
                        glVertex3fv(vertex)
                glColor3f(1.0, 1.0, 1.0)
        glEnd()

    def setupLighting(self):
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)
        ambient_color = [0.4, 0.4, 0.4, 1.0]
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color)
        diffuse_color = [1.2, 1.2, 1.2, 1.0]
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color)
        light_position = [1.0, 1.0, 1.0, 1.0]
        glLightfv(GL_LIGHT0, GL_POSITION, light_position)


class GraphicsPipelineView(QDialog):
    def __init__(self):
        super(GraphicsPipelineView, self).__init__()
        uic.loadUi("GraphicsPipelineView.ui", self)


class VulkanSetupGUI(QMainWindow):
    def __init__(self):
        super(VulkanSetupGUI, self).__init__()
        uic.loadUi("VulkanSetupGUI.ui", self)
        # uic.loadUi("pipelineTestView.ui", self)
        self.loadFile = ""
        self.saveFile = ""

        self.connectActions()
        self.loadModelPreview()
        self.loadTexturePreview()

    def updatePreviews(self):
        self.loadTexturePreview()
        self.loadModelPreview()

    def loadTexturePreview(self):
        self.texturePreviewImage.setPixmap(QPixmap(self.textureFileInput.text()))
        pass

    def loadModelPreview(self):
        # Check if the model file path is empty
        if not self.modelFileInput.text():
            return

        # Get the number of widgets in the vertical layout
        widgetCount = self.verticalLayoutModelPreview.count()

        # Remove the existing placeholder widget if there is more than one widget
        if widgetCount > 1:
            # Retrieve the placeholder widget
            placeholderWidget = self.verticalLayoutModelPreview.itemAt(1).widget()

            # Remove the placeholder widget from the layout
            self.verticalLayoutModelPreview.removeWidget(placeholderWidget)

            # Delete the placeholder widget to free up memory
            placeholderWidget.deleteLater()

        # Retrieve the model file path from the line edit
        modelFilePath = self.modelFileInput.text()

        # Create a new OpenGLWidget for model preview
        modelPreviewWidget = OpenGLWidget(modelFilePath)
        modelPreviewWidget.setMinimumSize(255, 255)
        modelPreviewWidget.setMaximumSize(255, 255)

        # Add the model preview widget to the vertical layout
        self.verticalLayoutModelPreview.addWidget(modelPreviewWidget)

    def connectActions(self):
        # Menu Bar and Bottom
        self.actionExport_to_Vulkan.triggered.connect(self.setOutput)
        self.generateVulkanCodeButton.clicked.connect(self.setOutput)
        self.actionSaveToFile.triggered.connect(self.saveToFile)
        self.actionLoadFromFile.triggered.connect(self.loadFromFile)
        self.actionLoadFromFile.triggered.connect(self.loadTexturePreview)
        self.actionLoadFromFile.triggered.connect(self.loadModelPreview)

        # Instance

        # Physical Device

        # Logical Device
        self.addExtensionButton.clicked.connect(self.showAddExtensionInput)
        self.removeExtensionButton.clicked.connect(self.showRemoveExtension)

        # Swapchain

        # Model
        self.modelFileToolButton.clicked.connect(lambda: self.setFilePath("modelFileToolButton", None))
        self.textureFileToolButton.clicked.connect(lambda: self.setFilePath("textureFileToolButton", None))
        self.modelFileInput.returnPressed.connect(self.loadModelPreview)
        self.textureFileInput.returnPressed.connect(self.loadTexturePreview)

        # GraphicsPipeline
        self.addPipelineButton.clicked.connect(self.showAddPipelineInput)
        self.editPipelineButton.clicked.connect(self.showEditPipelineInput)
        self.deletePipelineButton.clicked.connect(self.showRemovePipeline)
        self.graphicsPipelinesList.itemDoubleClicked.connect(self.showEditPipelineInput)
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
        print("graphicsPipelinesList: ")
        for i in range(self.graphicsPipelinesList.count()):
            print("[{}]: [{}]".format(self.graphicsPipelinesList.item(i).data(0),
                                      self.graphicsPipelinesList.item(i).data(Qt.UserRole)))

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

    def showMissingInput(self, missing_inputs):
        d = QDialog(self)
        d.setWindowTitle("Missing Inputs")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel("The following inputs were missing:")
        layout.addWidget(label)

        if isinstance(missing_inputs, str):
            missing_inputs = [missing_inputs]

        for missing in missing_inputs:
            missing_label = QLabel(missing)
            layout.addWidget(missing_label)

        ok_button = QPushButton("OK")
        ok_button.clicked.connect(d.accept)
        layout.addWidget(ok_button)

        d.setLayout(layout)
        d.exec_()

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

        def addPipeline(pipeline):
            if not pipeline:
                self.showNotAllowedInput("empty line")
                return

            # Convert pipeline to a list object using ast.literal_eval
            try:
                pipeline = ast.literal_eval(pipeline)
            except (ValueError, SyntaxError) as e:
                print(f"Error converting pipeline: {e}")
                return

            pipelineItem = QListWidgetItem(f"Graphics Pipeline {str(self.graphicsPipelinesList.count() + 1)}")
            pipelineItem.setData(Qt.UserRole, pipeline)

            if self.addUniquePipeline(self.graphicsPipelinesList, pipelineItem):
                print(f"Pipeline Added: {pipeline}")
                self.checkPipelineInput()  # Perform validation

            else:
                showPipelineAlreadyPresent(pipeline)

        def addPipelineParameters():

            parameters = []
            parameters.append(view.vertexTopologyInput.currentText())
            parameters.append(view.primitiveRestartInput.currentText())
            parameters.append(view.depthClampInput.currentText())
            parameters.append(view.rasterizerDiscardInput.currentText())
            parameters.append(view.polygonModeInput.currentText())
            parameters.append(view.lineWidthInput.text())
            parameters.append(view.cullModeInput.currentText())
            parameters.append(view.frontFaceInput.currentText())
            parameters.append(view.depthBiasEnabledInput.currentText())
            parameters.append(view.slopeFactorInput.text())
            parameters.append(view.constantFactorInput.text())
            parameters.append(view.biasClampInput.text())
            parameters.append(view.depthTestInput.currentText())
            parameters.append(view.depthWriteInput.currentText())
            parameters.append(view.depthCompareOperationInput.currentText())
            parameters.append(view.depthBoundsTestInput.currentText())
            parameters.append(view.depthBoundsMinInput.text())
            parameters.append(view.depthBoundsMaxInput.text())
            parameters.append(view.stencilTestInput.currentText())
            parameters.append(view.sampleShadingInput.currentText())
            parameters.append(view.rasterizationSamplesInput.currentText())
            parameters.append(view.minSampleShadingInput.text())
            parameters.append(view.alphaToCoverageInput.currentText())
            parameters.append(view.alphaToOneInput.currentText())
            parameters.append(view.colorWriteMaskInput.currentText())
            parameters.append(view.colorBlendInput.currentText())
            parameters.append(view.sourceColorBlendFactorInput.currentText())
            parameters.append(view.destinationColorBlendFactorInput.currentText())
            parameters.append(view.colorBlendOperationInput.currentText())
            parameters.append(view.sourceAlphaBlendFactorInput.currentText())
            parameters.append(view.destinationAlphaBlendFactorInput.currentText())
            parameters.append(view.alphaBlendOperationInput.currentText())
            parameters.append(view.logicOperationEnabledInput.currentText())
            parameters.append(view.logicOperationInput.currentText())
            parameters.append(view.attachmentCountInput.text())
            parameters.append(view.blendConstant0Input.text())
            parameters.append(view.blendConstant1Input.text())
            parameters.append(view.blendConstant2Input.text())
            parameters.append(view.blendConstant3Input.text())
            parameters.append(view.vertexShaderFileInput.text())
            parameters.append(view.vertexShaderEntryFunctionNameInput.text())
            parameters.append(view.fragmentShaderFileInput.text())
            parameters.append(view.fragmentShaderEntryFunctionNameInput.text())
            parameters.append(convertToVulkanNaming(view.reduceSpirvCodeSizeCheckBox.isChecked()))
            parameters.append(convertToVulkanNaming(view.useIndexedVerticesCheckBox.isChecked()))

            return parameters

        view = GraphicsPipelineView()
        view.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        view.addPipelineOKButton.accepted.connect(lambda: addPipeline(str(addPipelineParameters())))
        view.vertexShaderFileToolButton.clicked.connect(lambda: self.setFilePath("vertexShaderFileToolButton", view))
        view.fragmentShaderFileToolButton.clicked.connect(
            lambda: self.setFilePath("fragmentShaderFileToolButton", view))
        view.exec_()

    def showEditPipelineInput(self):
        def updateGraphicsPipelineView(item):
            def editPipeline():
                selected_item = self.graphicsPipelinesList.currentItem()
                if selected_item is not None:
                    pipeline_data = editPipelineParameters()
                    selected_item.setData(Qt.UserRole, pipeline_data)
                    if self.checkPipelineInput():  # Perform validation
                        view.close()

            def editPipelineParameters():
                parameters = []
                parameters.append(view.vertexTopologyInput.currentText())
                parameters.append(view.primitiveRestartInput.currentText())
                parameters.append(view.depthClampInput.currentText())
                parameters.append(view.rasterizerDiscardInput.currentText())
                parameters.append(view.polygonModeInput.currentText())
                parameters.append(view.lineWidthInput.text())
                parameters.append(view.cullModeInput.currentText())
                parameters.append(view.frontFaceInput.currentText())
                parameters.append(view.depthBiasEnabledInput.currentText())
                parameters.append(view.slopeFactorInput.text())
                parameters.append(view.constantFactorInput.text())
                parameters.append(view.biasClampInput.text())
                parameters.append(view.depthTestInput.currentText())
                parameters.append(view.depthWriteInput.currentText())
                parameters.append(view.depthCompareOperationInput.currentText())
                parameters.append(view.depthBoundsTestInput.currentText())
                parameters.append(view.depthBoundsMinInput.text())
                parameters.append(view.depthBoundsMaxInput.text())
                parameters.append(view.stencilTestInput.currentText())
                parameters.append(view.sampleShadingInput.currentText())
                parameters.append(view.rasterizationSamplesInput.currentText())
                parameters.append(view.minSampleShadingInput.text())
                parameters.append(view.alphaToCoverageInput.currentText())
                parameters.append(view.alphaToOneInput.currentText())
                parameters.append(view.colorWriteMaskInput.currentText())
                parameters.append(view.colorBlendInput.currentText())
                parameters.append(view.sourceColorBlendFactorInput.currentText())
                parameters.append(view.destinationColorBlendFactorInput.currentText())
                parameters.append(view.colorBlendOperationInput.currentText())
                parameters.append(view.sourceAlphaBlendFactorInput.currentText())
                parameters.append(view.destinationAlphaBlendFactorInput.currentText())
                parameters.append(view.alphaBlendOperationInput.currentText())
                parameters.append(view.logicOperationEnabledInput.currentText())
                parameters.append(view.logicOperationInput.currentText())
                parameters.append(view.attachmentCountInput.text())
                parameters.append(view.blendConstant0Input.text())
                parameters.append(view.blendConstant1Input.text())
                parameters.append(view.blendConstant2Input.text())
                parameters.append(view.blendConstant3Input.text())
                parameters.append(view.vertexShaderFileInput.text())
                parameters.append(view.vertexShaderEntryFunctionNameInput.text())
                parameters.append(view.fragmentShaderFileInput.text())
                parameters.append(view.fragmentShaderEntryFunctionNameInput.text())
                parameters.append(convertToVulkanNaming(view.reduceSpirvCodeSizeCheckBox.isChecked()))
                parameters.append(convertToVulkanNaming(view.useIndexedVerticesCheckBox.isChecked()))

                return parameters

            view = GraphicsPipelineView()
            view.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
            pipeline_data = self.graphicsPipelinesList.currentItem().data(Qt.UserRole)

            # Update the pipeline data in the view's fields
            view.vertexTopologyInput.setCurrentText(pipeline_data[0])
            view.primitiveRestartInput.setCurrentText(pipeline_data[1])
            view.depthClampInput.setCurrentText(pipeline_data[2])
            view.rasterizerDiscardInput.setCurrentText(pipeline_data[3])
            view.polygonModeInput.setCurrentText(pipeline_data[4])
            view.lineWidthInput.setValue(float(pipeline_data[5].replace(',', '.')))
            view.cullModeInput.setCurrentText(pipeline_data[6])
            view.frontFaceInput.setCurrentText(pipeline_data[7])
            view.depthBiasEnabledInput.setCurrentText(pipeline_data[8])
            view.slopeFactorInput.setValue(float(pipeline_data[9].replace(',', '.')))
            view.constantFactorInput.setValue(float(pipeline_data[10].replace(',', '.')))
            view.biasClampInput.setValue(float(pipeline_data[11].replace(',', '.')))
            view.depthTestInput.setCurrentText(pipeline_data[12])
            view.depthWriteInput.setCurrentText(pipeline_data[13])
            view.depthCompareOperationInput.setCurrentText(pipeline_data[14])
            view.depthBoundsTestInput.setCurrentText(pipeline_data[15])
            view.depthBoundsMinInput.setValue(float(pipeline_data[16].replace(',', '.')))
            view.depthBoundsMaxInput.setValue(float(pipeline_data[17].replace(',', '.')))
            view.stencilTestInput.setCurrentText(pipeline_data[18])
            view.sampleShadingInput.setCurrentText(pipeline_data[19])
            view.rasterizationSamplesInput.setCurrentText(pipeline_data[20])
            view.minSampleShadingInput.setValue(float(pipeline_data[21].replace(',', '.')))
            view.alphaToCoverageInput.setCurrentText(pipeline_data[22])
            view.alphaToOneInput.setCurrentText(pipeline_data[23])
            view.colorWriteMaskInput.setCurrentText(pipeline_data[24])
            view.colorBlendInput.setCurrentText(pipeline_data[25])
            view.sourceColorBlendFactorInput.setCurrentText(pipeline_data[26])
            view.destinationColorBlendFactorInput.setCurrentText(pipeline_data[27])
            view.colorBlendOperationInput.setCurrentText(pipeline_data[28])
            view.sourceAlphaBlendFactorInput.setCurrentText(pipeline_data[29])
            view.destinationAlphaBlendFactorInput.setCurrentText(pipeline_data[30])
            view.alphaBlendOperationInput.setCurrentText(pipeline_data[31])
            view.logicOperationEnabledInput.setCurrentText(pipeline_data[32])
            view.logicOperationInput.setCurrentText(pipeline_data[33])
            view.attachmentCountInput.setValue(int(pipeline_data[34]))
            view.blendConstant0Input.setValue(float(pipeline_data[35].replace(',', '.')))
            view.blendConstant1Input.setValue(float(pipeline_data[36].replace(',', '.')))
            view.blendConstant2Input.setValue(float(pipeline_data[37].replace(',', '.')))
            view.blendConstant3Input.setValue(float(pipeline_data[38].replace(',', '.')))
            view.vertexShaderFileInput.setText(pipeline_data[39])
            view.vertexShaderEntryFunctionNameInput.setText(pipeline_data[40])
            view.fragmentShaderFileInput.setText(pipeline_data[41])
            view.fragmentShaderEntryFunctionNameInput.setText(pipeline_data[42])
            view.reduceSpirvCodeSizeCheckBox.setChecked(convertFromVulkanNaming(pipeline_data[43]))
            view.useIndexedVerticesCheckBox.setChecked(convertFromVulkanNaming(pipeline_data[44]))

            view.addPipelineOKButton.accepted.connect(editPipeline)
            view.setWindowTitle("Edit Graphics Pipeline")
            view.vertexShaderFileToolButton.clicked.connect(
                lambda: self.setFilePath("vertexShaderFileToolButton", view))
            view.fragmentShaderFileToolButton.clicked.connect(
                lambda: self.setFilePath("fragmentShaderFileToolButton", view))
            view.exec_()

        selected_item = self.graphicsPipelinesList.currentItem()
        if selected_item is not None:
            updateGraphicsPipelineView(selected_item)

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

    def setFilePath(self, fileInput: str, referenceView):
        def setInputFilterBasedOnOrigin(fileInput):
            filterMap = {
                "modelFileToolButton": "Model files (*.obj)",
                "textureFileToolButton": "Texture image files (*.jpg, *.png)",
                "vertexShaderFileToolButton": "Vertex shader files (*.vert)",
                "fragmentShaderFileToolButton": "fragment shader files (*.frag)",
                "actionLoadFromFile": "Vulkan Setup GUI files (*.xml)"
            }
            return filterMap.get(fileInput, "")

        def setParametersBasedOnOrigin(fileInput, fileName):
            inputMap = {
                "modelFileToolButton": self.modelFileInput,
                "textureFileToolButton": self.textureFileInput,
                "vertexShaderFileToolButton": getattr(referenceView, "vertexShaderFileInput", None),
                "fragmentShaderFileToolButton": getattr(referenceView, "fragmentShaderFileInput", None),
                "actionLoadFromFile": "loadFile"  # assign value to self.loadFile
            }
            if fileInput not in inputMap:
                return
            inputField = inputMap[fileInput]
            if inputField != "" and fileName[0] == "":
                return
            if isinstance(inputField, QLineEdit):
                inputField.setText(fileName[0])
            else:
                setattr(self, inputMap[fileInput], fileName[0])  # assign value to e.g. self.loadFile

        inputFilter = setInputFilterBasedOnOrigin(fileInput)
        placeholderFilename = ''
        fileName = QFileDialog.getOpenFileName(self, 'Open file', placeholderFilename, inputFilter)
        setParametersBasedOnOrigin(fileInput, fileName)

        self.updatePreviews()

    ### Validation Section
    def checkInput(self):
        missing_inputs = []

        if len(self.applicationNameInput.text()) == 0:
            missing_inputs.append((self.applicationNameLabel.text()).replace(":", ""))

        if not self.checkDeviceExtensions():
            return False

        if not self.checkPipelineInput():
            return False

        if missing_inputs:
            self.showMissingInput(missing_inputs)
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

    def checkPipelineInput(self):
        # Iterate through all graphics pipelines
        graphicsPipelines = self.graphicsPipelinesList
        missing_inputs = []

        for i in range(graphicsPipelines.count()):
            item = graphicsPipelines.item(i)
            data = item.data(Qt.UserRole)
            pipelineName = item.data(0)

            vertexShaderFileInput = data[39]
            vertexShaderEntryFunctionNameInput = data[40]
            fragmentShaderFileInput = data[41]
            fragmentShaderEntryFunctionNameInput = data[42]

            if len(vertexShaderFileInput) == 0:
                missing_inputs.append(f"{pipelineName}: Vertex Shader File")

            if len(fragmentShaderFileInput) == 0:
                missing_inputs.append(f"{pipelineName}: Fragment Shader File")

            if len(vertexShaderEntryFunctionNameInput) == 0:
                missing_inputs.append(f"{pipelineName}: Vertex Shader Entry Function Name")

            if len(fragmentShaderEntryFunctionNameInput) == 0:
                missing_inputs.append(f"{pipelineName}: Fragment Shader Entry Function Name")

        if missing_inputs:
            self.showMissingInput(missing_inputs)
            return False

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

    def addUniquePipeline(self, listWidget, item):
        for i in range(listWidget.count()):
            if listWidget.item(i).data(Qt.UserRole) == item.data(Qt.UserRole):  # user roles can be used to hide data
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

    ### XML section
    def loadFromFile(self):
        self.setFilePath("actionLoadFromFile", None)
        fileName = self.loadFile
        if fileName:
            self.readXml(fileName)

    def saveToFile(self):
        # Show file dialog to select file storage location
        filePath, _ = QFileDialog.getSaveFileName(self, 'Save file', self.saveFile,
                                                  'Vulkan Setup GUI files (*.xml)')

        if filePath:
            # Append the specified filename to the file path
            filePath = os.path.join(os.path.dirname(filePath), self.saveFile + ".xml")

            self.writeXml(filePath)

    def writeXml(self, fileName):
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

        # Graphics Pipeline
        graphicsPipelines = ET.SubElement(root, 'graphicsPipelines')

        for i in range(self.graphicsPipelinesList.count()):
            item = self.graphicsPipelinesList.item(i)
            pipeline = ET.SubElement(graphicsPipelines, 'pipeline', name=f'{item.data(0)}')
            data = item.data(Qt.UserRole)

            pipeline_inputs = [
                'vertexTopologyInput', 'primitiveRestartInput', 'depthClampInput',
                'rasterizerDiscardInput', 'polygonModeInput', 'lineWidthInput',
                'cullModeInput', 'frontFaceInput', 'depthBiasEnabledInput',
                'slopeFactorInput', 'constantFactorInput', 'biasClampInput',
                'depthTestInput', 'depthWriteInput', 'depthCompareOperationInput',
                'depthBoundsTestInput', 'depthBoundsMinInput', 'depthBoundsMaxInput',
                'stencilTestInput', 'sampleShadingInput', 'rasterizationSamplesInput',
                'minSampleShadingInput', 'alphaToCoverageInput', 'alphaToOneInput',
                'colorWriteMaskInput', 'colorBlendInput',
                'sourceColorBlendFactorInput', 'destinationColorBlendFactorInput',
                'colorBlendOperationInput', 'sourceAlphaBlendFactorInput',
                'destinationAlphaBlendFactorInput', 'alphaBlendOperationInput',
                'logicOperationEnabledInput', 'logicOperationInput',
                'attachmentCountInput', 'blendConstant0Input', 'blendConstant1Input',
                'blendConstant2Input', 'blendConstant3Input', 'vertexShaderFileInput',
                'vertexShaderEntryFunctionNameInput', 'fragmentShaderFileInput',
                'fragmentShaderEntryFunctionNameInput', 'reduceSpirvCodeSizeCheckBox',
                'useIndexedVerticesCheckBox'
            ]

            for index, value in enumerate(data):
                input_name = pipeline_inputs[index]
                ET.SubElement(pipeline, input_name, name=input_name).text = str(value)

        # create a new XML file with the results
        mydata = ET.tostring(root)
        with open(fileName, "wb") as myfile:
            myfile.write(mydata)

        print("XML File created with parameters:")
        self.printInputs()

    def readXml(self, fileName):
        # tree = ET.parse('VulkanSetup.xml')
        tree = ET.parse(fileName)
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
                self.modelFileInput.setText(elem.text.strip())
            if elem.tag == "textureFileInput":
                self.textureFileInput.setText(elem.text.strip())

            # Graphics Pipeline
            if elem.tag == "pipeline":
                pipelineName = elem.attrib["name"]
                pipeline = []

                for child in elem:
                    if child.tag in {"vertexShaderFileInput", "vertexShaderEntryFunctionNameInput",
                                     "fragmentShaderFileInput",
                                     "fragmentShaderEntryFunctionNameInput"} and child.text is None:
                        pipeline.append("")
                    else:
                        pipeline.append(child.text.strip())

                pipelineItem = QListWidgetItem(pipelineName)
                pipelineItem.setData(Qt.UserRole, pipeline)
                if self.addUniquePipeline(self.graphicsPipelinesList, pipelineItem):
                    print(f"Pipeline Added: {pipeline}")

            for subelem in elem:
                traverse(subelem)

            for subelem in elem:
                traverse(subelem)

        self.graphicsPipelinesList.clear()
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
