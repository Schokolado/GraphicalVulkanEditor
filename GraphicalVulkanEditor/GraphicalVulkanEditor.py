import ast
import os

from PyQt5 import uic
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPixmap, QVector3D
from pywavefront import Wavefront
from PyQt5.QtWidgets import *
from OpenGL.GL import *
import xml.etree.ElementTree as ET

DEFAULT_DEVICE_EXTENSIONS = ["VK_KHR_SWAPCHAIN_EXTENSION_NAME"]
VULKAN_HEADER_OUTPUT_LOCATION = "../GraphicalVulkanEditorProjectVariables.h"

class OpenGLWidget(QOpenGLWidget):
    def __init__(self, OBJFilePath, parent=None):
        super().__init__(parent)
        self.OBJFilePath = OBJFilePath
        self.OBJMesh = None

    def setOBJFilePath(self, path):
        self.OBJFilePath = path

    def initializeGL(self):
        glClearColor(0.0, 0.0, 0.0, 1.0)
        self.OBJMesh = Wavefront(self.OBJFilePath, collect_faces=True)
        self.setupLighting()

    def resizeGL(self, width, height):
        glViewport(0, 0, width, height)

    def updateGL(self):
        self.OBJMesh = Wavefront(self.OBJFilePath, collect_faces=True)
        self.paintGL()

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glEnable(GL_DEPTH_TEST)

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        rotation_angle = 90  # Rotation angle in degrees
        rotation_axis = QVector3D(-1, 0, 0)  # Rotation axis (e.g., Y-axis)
        glRotatef(rotation_angle, rotation_axis.x(), rotation_axis.y(), rotation_axis.z())

        #translation = QVector3D(0, 0, -0.3)  # Translation vector (e.g., move 0.2 units down on the Y-axis)
        #glTranslate(translation.x(), translation.y(), translation.z())

        glBegin(GL_TRIANGLES)
        if self.OBJMesh:
            for mesh in self.OBJMesh.mesh_list:
                for face in mesh.faces:
                    for vertex_i in face:
                        vertex = self.OBJMesh.vertices[vertex_i]
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

class OpenGLPreviewWidget(QOpenGLWidget):
    def __init__(self, OBJFilePath, parent=None,
                 rasterizerInfo_depthClampEnable=False,
                 rasterizerInfo_polygonMode=GL_FILL,
                 rasterizerInfo_lineWidth=1.0,
                 rasterizerInfo_cullMode=GL_BACK,
                 rasterizerInfo_frontFace=GL_CCW,
                 depthStencilInfo_depthTestEnable=True,
                 depthStencilInfo_depthWriteEnable=True,
                 depthStencilInfo_depthCompareOp=GL_LESS,
                 multisamplingInfo_sampleShadingEnable=False,
                 multisamplingInfo_rasterizationSamples=1,
                 colorBlendAttachment_colorWriteMask=GL_TRUE,
                 colorBlendAttachment_blendEnable=False,
                 colorBlendAttachment_srcColorBlendFactor=GL_ONE,
                 colorBlendAttachment_dstColorBlendFactor=GL_ZERO,
                 colorBlendAttachment_colorBlendOp=GL_FUNC_ADD,
                 colorBlendingInfo_logicOpEnable=False,
                 colorBlendingInfo_logicOp=GL_COPY,
                 colorBlendingInfo_blendConstants_0=0.0,
                 colorBlendingInfo_blendConstants_1=0.0,
                 colorBlendingInfo_blendConstants_2=0.0,
                 colorBlendingInfo_blendConstants_3=0.0):
        super().__init__(parent)
        self.OBJFilePath = OBJFilePath
        self.OBJMesh = None

        self.rasterizerInfo_depthClampEnable = rasterizerInfo_depthClampEnable
        self.rasterizerInfo_polygonMode = self.mapPolygonMode(rasterizerInfo_polygonMode)
        self.rasterizerInfo_lineWidth = rasterizerInfo_lineWidth
        self.rasterizerInfo_cullMode = self.mapCullMode(rasterizerInfo_cullMode)
        self.rasterizerInfo_frontFace = self.mapFrontFace(rasterizerInfo_frontFace)
        self.depthStencilInfo_depthTestEnable = self.convertFromVulkanNaming(depthStencilInfo_depthTestEnable)
        self.depthStencilInfo_depthWriteEnable = self.convertFromVulkanNaming(depthStencilInfo_depthWriteEnable)
        self.depthStencilInfo_depthCompareOp = self.mapCompareOp(depthStencilInfo_depthCompareOp)
        self.multisamplingInfo_sampleShadingEnable = self.convertFromVulkanNaming(multisamplingInfo_sampleShadingEnable)
        self.multisamplingInfo_rasterizationSamples = self.mapSampleCount(multisamplingInfo_rasterizationSamples)
        self.colorBlendAttachment_colorWriteMask = self.convertColorBlendAttachment_colorWriteMask(colorBlendAttachment_colorWriteMask)
        self.colorBlendAttachment_blendEnable = self.convertFromVulkanNaming(colorBlendAttachment_blendEnable)
        self.colorBlendAttachment_srcColorBlendFactor = self.mapBlendFactor(colorBlendAttachment_srcColorBlendFactor)
        self.colorBlendAttachment_dstColorBlendFactor = self.mapBlendFactor(colorBlendAttachment_dstColorBlendFactor)
        self.colorBlendAttachment_colorBlendOp = self.mapBlendOp(colorBlendAttachment_colorBlendOp)
        self.colorBlendingInfo_logicOpEnable = self.convertFromVulkanNaming(colorBlendingInfo_logicOpEnable)
        self.colorBlendingInfo_logicOp = self.mapLogicOp(colorBlendingInfo_logicOp)
        self.colorBlendingInfo_blendConstants_0 = colorBlendingInfo_blendConstants_0
        self.colorBlendingInfo_blendConstants_1 = colorBlendingInfo_blendConstants_1
        self.colorBlendingInfo_blendConstants_2 = colorBlendingInfo_blendConstants_2
        self.colorBlendingInfo_blendConstants_3 = colorBlendingInfo_blendConstants_3

    def initializeGL(self):
        glClearColor(0.0, 0.0, 0.0, 1.0)
        self.OBJMesh = Wavefront(self.OBJFilePath, collect_faces=True)
        self.setupLighting()

    def resizeGL(self, width, height):
        glViewport(0, 0, width, height)

    def updateGL(self):
        self.OBJMesh = Wavefront(self.OBJFilePath, collect_faces=True)
        self.paintGL()

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glEnable(GL_DEPTH_TEST)

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        rotation_angle = 90  # Rotation angle in degrees
        rotation_axis = QVector3D(-1, 0, 0)  # Rotation axis (e.g., Y-axis)
        glRotatef(rotation_angle, rotation_axis.x(), rotation_axis.y(), rotation_axis.z())

        if self.rasterizerInfo_depthClampEnable:
            glEnable(GL_DEPTH_CLAMP)
        else:
            glDisable(GL_DEPTH_CLAMP)

        glPolygonMode(GL_FRONT_AND_BACK, self.rasterizerInfo_polygonMode)
        glLineWidth(self.rasterizerInfo_lineWidth)

        if self.rasterizerInfo_cullMode == GL_NONE:
            glDisable(GL_CULL_FACE)
        else:
            glEnable(GL_CULL_FACE)
            glCullFace(self.rasterizerInfo_cullMode)

        glFrontFace(self.rasterizerInfo_frontFace)

        if self.depthStencilInfo_depthTestEnable:
            glEnable(GL_DEPTH_TEST)
        else:
            glDisable(GL_DEPTH_TEST)

        glDepthMask(self.depthStencilInfo_depthWriteEnable)

        glDepthFunc(self.depthStencilInfo_depthCompareOp)

        if self.multisamplingInfo_sampleShadingEnable:
            glEnable(GL_SAMPLE_SHADING)
        else:
            glDisable(GL_SAMPLE_SHADING)

        glSampleCoverage(1.0, self.multisamplingInfo_rasterizationSamples)

        colorMask = self.colorBlendAttachment_colorWriteMask
        glColorMask(*colorMask)

        if self.colorBlendAttachment_blendEnable:
            glEnable(GL_BLEND)
        else:
            glDisable(GL_BLEND)

        glBlendFunc(
            self.colorBlendAttachment_srcColorBlendFactor,
            self.colorBlendAttachment_dstColorBlendFactor
        )

        glBlendEquation(self.colorBlendAttachment_colorBlendOp)

        if self.colorBlendingInfo_logicOpEnable:
            glEnable(GL_COLOR_LOGIC_OP)
        else:
            glDisable(GL_COLOR_LOGIC_OP)

        glLogicOp(self.colorBlendingInfo_logicOp)

        glBlendColor(
            self.colorBlendingInfo_blendConstants_0,
            self.colorBlendingInfo_blendConstants_1,
            self.colorBlendingInfo_blendConstants_2,
            self.colorBlendingInfo_blendConstants_3
        )

        glBegin(GL_TRIANGLES)
        if self.OBJMesh:
            for mesh in self.OBJMesh.mesh_list:
                for face in mesh.faces:
                    for vertex_i in face:
                        vertex = self.OBJMesh.vertices[vertex_i]
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

    def convertColorBlendAttachment_colorWriteMask(self, mask: str):
        mapping = {
            "R": [GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE],
            "G": [GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE],
            "B": [GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE],
            "A": [GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE],
            "RG": [GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE],
            "RB": [GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE],
            "RA": [GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE],
            "GB": [GL_FALSE, GL_TRUE, GL_TRUE, GL_FALSE],
            "GA": [GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE],
            "BA": [GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE],
            "RGB": [GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE],
            "RGA": [GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE],
            "RBA": [GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE],
            "GBA": [GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE],
            "RGBA": [GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE]
        }
        return mapping.get(mask, [GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE])

    def mapTopology(self, topology):
        if topology == "VK_PRIMITIVE_TOPOLOGY_POINT_LIST":
            return GL_POINTS
        elif topology == "VK_PRIMITIVE_TOPOLOGY_LINE_LIST":
            return GL_LINES
        elif topology == "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP":
            return GL_LINE_STRIP
        elif topology == "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST":
            return GL_TRIANGLES
        elif topology == "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP":
            return GL_TRIANGLE_STRIP
        elif topology == "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN":
            return GL_TRIANGLE_FAN
        elif topology == "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY":
            return GL_LINES_ADJACENCY
        elif topology == "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY":
            return GL_LINE_STRIP_ADJACENCY
        elif topology == "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY":
            return GL_TRIANGLES_ADJACENCY
        elif topology == "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY":
            return GL_TRIANGLE_STRIP_ADJACENCY
        elif topology == "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST":
            return GL_PATCHES
        else:
            return GL_INVALID_ENUM  # Handle unknown topology values

    def mapPolygonMode(self, polygonMode):
        if polygonMode == "VK_POLYGON_MODE_FILL":
            return GL_FILL
        elif polygonMode == "VK_POLYGON_MODE_POINT":
            return GL_POINT
        elif polygonMode == "VK_POLYGON_MODE_LINE":
            return GL_LINE
        else:
            return GL_INVALID_ENUM  # Handle unknown polygon mode values

    def mapCullMode(self, cullMode):
        if cullMode == "VK_CULL_MODE_NONE":
            return GL_NONE
        elif cullMode == "VK_CULL_MODE_FRONT_BIT":
            return GL_FRONT
        elif cullMode == "VK_CULL_MODE_BACK_BIT":
            return GL_BACK
        elif cullMode == "VK_CULL_MODE_FRONT_AND_BACK":
            return GL_FRONT_AND_BACK
        else:
            return GL_INVALID_ENUM  # Handle unknown cull mode values

    def mapFrontFace(self, frontFace):
        if frontFace == "VK_FRONT_FACE_COUNTER_CLOCKWISE":
            return GL_CCW
        elif frontFace == "VK_FRONT_FACE_CLOCKWISE":
            return GL_CW
        else:
            return GL_INVALID_ENUM  # Handle unknown front face values

    def mapCompareOp(self, compareOp):
        if compareOp == "VK_COMPARE_OP_LESS":
            return GL_LESS
        elif compareOp == "VK_COMPARE_OP_GREATER":
            return GL_GREATER
        elif compareOp == "VK_COMPARE_OP_EQUAL":
            return GL_EQUAL
        elif compareOp == "VK_COMPARE_OP_NOT_EQUAL":
            return GL_NOTEQUAL
        elif compareOp == "VK_COMPARE_OP_LESS_OR_EQUAL":
            return GL_LEQUAL
        elif compareOp == "VK_COMPARE_OP_GREATER_OR_EQUAL":
            return GL_GEQUAL
        elif compareOp == "VK_COMPARE_OP_ALWAYS":
            return GL_ALWAYS
        elif compareOp == "VK_COMPARE_OP_NEVER":
            return GL_NEVER
        else:
            return GL_INVALID_ENUM  # Handle unknown compare operation values

    def mapBlendFactor(self, blendFactor):
        if blendFactor == "VK_BLEND_FACTOR_ZERO":
            return GL_ZERO
        elif blendFactor == "VK_BLEND_FACTOR_ONE":
            return GL_ONE
        elif blendFactor == "VK_BLEND_FACTOR_SRC_COLOR":
            return GL_SRC_COLOR
        elif blendFactor == "VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR":
            return GL_ONE_MINUS_SRC_COLOR
        elif blendFactor == "VK_BLEND_FACTOR_DST_COLOR":
            return GL_DST_COLOR
        elif blendFactor == "VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR":
            return GL_ONE_MINUS_DST_COLOR
        elif blendFactor == "VK_BLEND_FACTOR_SRC_ALPHA":
            return GL_SRC_ALPHA
        elif blendFactor == "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA":
            return GL_ONE_MINUS_SRC_ALPHA
        elif blendFactor == "VK_BLEND_FACTOR_DST_ALPHA":
            return GL_DST_ALPHA
        elif blendFactor == "VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA":
            return GL_ONE_MINUS_DST_ALPHA
        elif blendFactor == "VK_BLEND_FACTOR_CONSTANT_COLOR":
            return GL_CONSTANT_COLOR
        elif blendFactor == "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR":
            return GL_ONE_MINUS_CONSTANT_COLOR
        elif blendFactor == "VK_BLEND_FACTOR_CONSTANT_ALPHA":
            return GL_CONSTANT_ALPHA
        elif blendFactor == "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA":
            return GL_ONE_MINUS_CONSTANT_ALPHA
        elif blendFactor == "VK_BLEND_FACTOR_SRC_ALPHA_SATURATE":
            return GL_SRC_ALPHA_SATURATE
        else:
            return GL_INVALID_ENUM  # Handle unknown blend factor values

    def mapBlendOp(self, blendOp):
        if blendOp == "VK_BLEND_OP_ADD":
            return GL_FUNC_ADD
        elif blendOp == "VK_BLEND_OP_SUBTRACT":
            return GL_FUNC_SUBTRACT
        elif blendOp == "VK_BLEND_OP_REVERSE_SUBTRACT":
            return GL_FUNC_REVERSE_SUBTRACT
        elif blendOp == "VK_BLEND_OP_MIN":
            return GL_MIN
        elif blendOp == "VK_BLEND_OP_MAX":
            return GL_MAX
        else:
            return GL_INVALID_ENUM  # Handle unknown blend operation values

    def mapLogicOp(self, logicOp):
        if logicOp == "VK_LOGIC_OP_CLEAR":
            return GL_CLEAR
        elif logicOp == "VK_LOGIC_OP_AND":
            return GL_AND
        elif logicOp == "VK_LOGIC_OP_AND_REVERSE":
            return GL_AND_REVERSE
        elif logicOp == "VK_LOGIC_OP_COPY":
            return GL_COPY
        elif logicOp == "VK_LOGIC_OP_AND_INVERTED":
            return GL_AND_INVERTED
        elif logicOp == "VK_LOGIC_OP_NO_OP":
            return GL_NOOP
        elif logicOp == "VK_LOGIC_OP_XOR":
            return GL_XOR
        elif logicOp == "VK_LOGIC_OP_OR":
            return GL_OR
        elif logicOp == "VK_LOGIC_OP_NOR":
            return GL_NOR
        elif logicOp == "VK_LOGIC_OP_EQUIVALENT":
            return GL_EQUIV
        elif logicOp == "VK_LOGIC_OP_INVERT":
            return GL_INVERT
        elif logicOp == "VK_LOGIC_OP_OR_REVERSE":
            return GL_OR_REVERSE
        elif logicOp == "VK_LOGIC_OP_COPY_INVERTED":
            return GL_COPY_INVERTED
        elif logicOp == "VK_LOGIC_OP_OR_INVERTED":
            return GL_OR_INVERTED
        elif logicOp == "VK_LOGIC_OP_NAND":
            return GL_NAND
        elif logicOp == "VK_LOGIC_OP_SET":
            return GL_SET
        else:
            return GL_INVALID_ENUM  # Handle unknown logic operation values

    def mapSampleCount(self, sampleCount):
        if sampleCount == "VK_SAMPLE_COUNT_1_BIT":
            return 1
        elif sampleCount == "VK_SAMPLE_COUNT_2_BIT":
            return 2
        elif sampleCount == "VK_SAMPLE_COUNT_4_BIT":
            return 4
        elif sampleCount == "VK_SAMPLE_COUNT_8_BIT":
            return 8
        elif sampleCount == "VK_SAMPLE_COUNT_16_BIT":
            return 16
        elif sampleCount == "VK_SAMPLE_COUNT_32_BIT":
            return 32
        elif sampleCount == "VK_SAMPLE_COUNT_64_BIT":
            return 64
        else:
            return -1  # Handle unknown sample count values

    def convertFromVulkanNaming(self, input):
        if input == "VK_TRUE":
            return True
        elif input == "VK_FALSE":
            return False
        else:
            return input


class GraphicsPipelineView(QDialog):
    def __init__(self):
        super(GraphicsPipelineView, self).__init__()
        uic.loadUi("ui_views/GraphicsPipelineView.ui", self)


class GraphicalVulkanEditor(QMainWindow):
    def __init__(self):
        super(GraphicalVulkanEditor, self).__init__()
        uic.loadUi("ui_views/GraphicalVulkanEditor.ui", self)
        self.loadFile = ""
        self.saveFile = ""

        self.connectActions()
        self.loadModelPreview()
        self.loadTexturePreview()

    #################################
    ######## Utility section ########
    #################################

    # Conversion Functions
    def convertToVulkanNaming(self, input: str):
        if type(input) == bool:
            if input:
                return "VK_TRUE"
            if not input:
                return "VK_FALSE"
        else:
            print(f"Provided input was [{input}]. Unnecessary conversion call.")
            return input

    def convertFromVulkanNaming(self, input):
        if input == "VK_TRUE":
            return True
        elif input == "VK_FALSE":
            return False
        else:
            return input

    # Preview functions
    def updatePreviews(self):
        self.loadTexturePreview()
        self.loadModelPreview()

    def loadTexturePreview(self):
        self.texturePreviewImage.setPixmap(QPixmap(self.textureFileInput.text()))
        pass

    def loadModelPreview(self):
        # Check if the model file path is empty
        if not self.modelFileInput.text() or not os.path.isfile(self.modelFileInput.text()):
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

    def loadPipelinePreview(self):
        # Check if the model file path is empty
        if not self.modelFileInput.text() or not os.path.isfile(self.modelFileInput.text()):
            return

        # Get the selected item from the graphics pipelines list
        selectedItems = self.graphicsPipelinesList.selectedItems()
        if not selectedItems:
            return

        # Get the number of widgets in the vertical layout
        widgetCount = self.verticalLayoutPipelinePreview.count()

        # Remove the existing placeholder widget if there is more than one widget
        if widgetCount > 4:
            # Retrieve the placeholder widget
            placeholderWidget = self.verticalLayoutPipelinePreview.itemAt(4).widget()

            # Remove the placeholder widget from the layout
            self.verticalLayoutPipelinePreview.removeWidget(placeholderWidget)

            # Delete the placeholder widget to free up memory
            placeholderWidget.deleteLater()

        # Retrieve the model file path from the line edit
        modelFilePath = self.modelFileInput.text()



        # Retrieve the selected item data
        selectedPipelineData = selectedItems[0].data(Qt.UserRole)

        # Create a new OpenGLWidget for model preview using the selected pipeline parameters
        parameters = {
            "rasterizerInfo_depthClampEnable": selectedPipelineData[2],
            "rasterizerInfo_polygonMode": selectedPipelineData[4],
            "rasterizerInfo_lineWidth": float(selectedPipelineData[5].replace(',', '.')),
            "rasterizerInfo_cullMode": selectedPipelineData[6],
            "rasterizerInfo_frontFace": selectedPipelineData[7],
            "depthStencilInfo_depthTestEnable": selectedPipelineData[12],
            "depthStencilInfo_depthWriteEnable": selectedPipelineData[13],
            "depthStencilInfo_depthCompareOp": selectedPipelineData[14],
            "multisamplingInfo_sampleShadingEnable": selectedPipelineData[19],
            "multisamplingInfo_rasterizationSamples": selectedPipelineData[20],
            "colorBlendAttachment_colorWriteMask": selectedPipelineData[24],
            "colorBlendAttachment_blendEnable": selectedPipelineData[25],
            "colorBlendAttachment_srcColorBlendFactor": selectedPipelineData[26],
            "colorBlendAttachment_dstColorBlendFactor": selectedPipelineData[27],
            "colorBlendAttachment_colorBlendOp": selectedPipelineData[28],
            "colorBlendingInfo_logicOpEnable": selectedPipelineData[32],
            "colorBlendingInfo_logicOp": selectedPipelineData[33],
            "colorBlendingInfo_blendConstants_0": float(selectedPipelineData[35].replace(',', '.')),
            "colorBlendingInfo_blendConstants_1": float(selectedPipelineData[36].replace(',', '.')),
            "colorBlendingInfo_blendConstants_2": float(selectedPipelineData[37].replace(',', '.')),
            "colorBlendingInfo_blendConstants_3": float(selectedPipelineData[38].replace(',', '.')),
        }

        pipelinePreviewWidget = OpenGLPreviewWidget(modelFilePath, **parameters)
        pipelinePreviewWidget.setMinimumSize(255, 255)
        pipelinePreviewWidget.setMaximumSize(255, 255)

        # Add the model preview widget to the vertical layout
        self.verticalLayoutPipelinePreview.addWidget(pipelinePreviewWidget)

    #################################
    ##### Connection Section ########
    #################################

    def connectActions(self):
        # Menu Bar and Bottom
        self.actionExportToVulkan.triggered.connect(self.setOutput)
        self.generateVulkanCodeButton.clicked.connect(self.setOutput)
        self.actionSaveToFile.triggered.connect(self.saveToXMLFile)
        self.actionLoadFromFile.triggered.connect(self.loadFromXMLFile)
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
        self.pipelinePreviewButton.clicked.connect(self.loadPipelinePreview)

    #################################
    ######## Print section ##########
    #################################

    def setOutput(self):
        # Check if all input is set and checked
        if not self.checkInput():
            return

        self.printInputs()
        if self.writeVulkanHeader():
            self.showHeaderFileCreated()
        else:
            self.showHeaderFileCreationFailed()


    def printInputs(self):
        # Instance
        print("applicationNameInput: [{}]".format(
            self.applicationNameInput.text()))
        print("showValidationLayerDebugInfoCheckBox: [{}]".format(
            self.convertToVulkanNaming(self.showValidationLayerDebugInfoCheckBox.isChecked())))
        print("runOnMacosCheckBox: [{}]".format(
            self.convertToVulkanNaming(self.runOnMacosCheckBox.isChecked())))

        # Physical Device
        print("chooseGPUOnStartupCheckBox: [{}]".format(
            self.convertToVulkanNaming(self.chooseGPUOnStartupCheckBox.isChecked())))

        # Logical Device
        print("deviceExtensionsList: [{}]".format(
            self.getListContents(self.deviceExtensionsList)))

        # Swapchain
        print("imageHeightInput: [{}]".format(
            self.imageHeightInput.text()))
        print("imageWidthInput: [{}]".format(
            self.imageWidthInput.text()))
        print("lockWindowSizeCheckBox: [{}]".format(
            self.convertToVulkanNaming(self.lockWindowSizeCheckBox.isChecked())))
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
            self.convertToVulkanNaming(self.saveEnergyForMobileCheckBox.isChecked())))
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
        print("useIndexedVerticesCheckBox: [{}]".format(
            self.convertToVulkanNaming(self.useIndexedVerticesCheckBox.isChecked())))
        print("reduceSpirvCodeSizeCheckBox: [{}]".format(
            self.convertToVulkanNaming(self.reduceSpirvCodeSizeCheckBox.isChecked())))
        print("graphicsPipelinesList: ")
        for i in range(self.graphicsPipelinesList.count()):
            print("[{}]: [{}]".format(self.graphicsPipelinesList.item(i).data(Qt.DisplayRole),
                                      self.graphicsPipelinesList.item(i).data(Qt.UserRole)))

        print()

    #################################
    ###### Show views section #######
    #################################

    def showHeaderFileCreated(self):
        d = QDialog()
        d.setWindowTitle("")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Vulkan project header file has been created under\n{VULKAN_HEADER_OUTPUT_LOCATION}")
        layout.addWidget(label)
        okButton = QPushButton("OK")
        okButton.clicked.connect(d.accept)
        layout.addWidget(okButton)
        d.setLayout(layout)
        d.exec_()

    def showHeaderFileCreationFailed(self):
        d = QDialog()
        d.setWindowTitle("")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Failed to create Vulkan project header file under\n{VULKAN_HEADER_OUTPUT_LOCATION}")
        layout.addWidget(label)
        okButton = QPushButton("OK")
        okButton.clicked.connect(d.accept)
        layout.addWidget(okButton)
        d.setLayout(layout)
        d.exec_()
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
        OKButton = QPushButton("OK")
        OKButton.clicked.connect(lambda: removeExtensions(self.deviceExtensionsList.selectedItems()))
        OKButton.clicked.connect(d.accept)
        layout.addWidget(OKButton)
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
            OKButton = QPushButton("OK")
            OKButton.clicked.connect(d.accept)
            layout.addWidget(OKButton)
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
        OKButton = QPushButton("Add")
        OKButton.clicked.connect(lambda: addExtension(lineEdit.text()))
        OKButton.clicked.connect(d.accept)
        layout.addWidget(OKButton)
        d.setLayout(layout)
        d.show()

    def showNotAllowedInput(self, notAllowed):
        d = QDialog(self)
        d.setWindowTitle(" ")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel(f"Input not allowed:\n\n\"{notAllowed}\"\n")
        layout.addWidget(label)
        OKButton = QPushButton("OK")
        OKButton.clicked.connect(d.accept)
        layout.addWidget(OKButton)
        d.setLayout(layout)
        d.show()

    def showMissingInput(self, missingInputs):
        d = QDialog(self)
        d.setWindowTitle("Missing Inputs")
        d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
        layout = QVBoxLayout()
        label = QLabel("The following inputs were missing:")
        layout.addWidget(label)

        if isinstance(missingInputs, str):
            missingInputs = [missingInputs]

        for missing in missingInputs:
            missingLabel = QLabel(missing)
            layout.addWidget(missingLabel)

        OKButton = QPushButton("OK")
        OKButton.clicked.connect(d.accept)
        layout.addWidget(OKButton)

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
        OKButton = QPushButton("OK")
        OKButton.clicked.connect(d.accept)
        layout.addWidget(OKButton)
        d.setLayout(layout)
        d.show()

    def showAddPipelineInput(self):
        def showPipelineAlreadyPresent(pipeline: str):
            d = QDialog(self)
            d.setWindowTitle("")
            d.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
            layout = QVBoxLayout()

            labelText = "Pipeline already present:\n"
            for element in pipeline:
                if element:
                    labelText += element + "\n"
                else:
                    labelText += "[No input provided]\n"
            label = QLabel(labelText)

            layout.addWidget(label)
            OKButton = QPushButton("OK")
            OKButton.clicked.connect(d.accept)
            layout.addWidget(OKButton)
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
                selectedItem = self.graphicsPipelinesList.currentItem()
                if selectedItem is not None:
                    data = editPipelineParameters()
                    selectedItem.setData(Qt.UserRole, data)
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

                return parameters

            view = GraphicsPipelineView()
            view.setWindowFlag(Qt.WindowContextHelpButtonHint, False)
            pipelineData = self.graphicsPipelinesList.currentItem().data(Qt.UserRole)

            # Update the pipeline data in the view's fields
            view.vertexTopologyInput.setCurrentText(pipelineData[0])
            view.primitiveRestartInput.setCurrentText(pipelineData[1])
            view.depthClampInput.setCurrentText(pipelineData[2])
            view.rasterizerDiscardInput.setCurrentText(pipelineData[3])
            view.polygonModeInput.setCurrentText(pipelineData[4])
            view.lineWidthInput.setValue(float(pipelineData[5].replace(',', '.')))
            view.cullModeInput.setCurrentText(pipelineData[6])
            view.frontFaceInput.setCurrentText(pipelineData[7])
            view.depthBiasEnabledInput.setCurrentText(pipelineData[8])
            view.slopeFactorInput.setValue(float(pipelineData[9].replace(',', '.')))
            view.constantFactorInput.setValue(float(pipelineData[10].replace(',', '.')))
            view.biasClampInput.setValue(float(pipelineData[11].replace(',', '.')))
            view.depthTestInput.setCurrentText(pipelineData[12])
            view.depthWriteInput.setCurrentText(pipelineData[13])
            view.depthCompareOperationInput.setCurrentText(pipelineData[14])
            view.depthBoundsTestInput.setCurrentText(pipelineData[15])
            view.depthBoundsMinInput.setValue(float(pipelineData[16].replace(',', '.')))
            view.depthBoundsMaxInput.setValue(float(pipelineData[17].replace(',', '.')))
            view.stencilTestInput.setCurrentText(pipelineData[18])
            view.sampleShadingInput.setCurrentText(pipelineData[19])
            view.rasterizationSamplesInput.setCurrentText(pipelineData[20])
            view.minSampleShadingInput.setValue(float(pipelineData[21].replace(',', '.')))
            view.alphaToCoverageInput.setCurrentText(pipelineData[22])
            view.alphaToOneInput.setCurrentText(pipelineData[23])
            view.colorWriteMaskInput.setCurrentText(pipelineData[24])
            view.colorBlendInput.setCurrentText(pipelineData[25])
            view.sourceColorBlendFactorInput.setCurrentText(pipelineData[26])
            view.destinationColorBlendFactorInput.setCurrentText(pipelineData[27])
            view.colorBlendOperationInput.setCurrentText(pipelineData[28])
            view.sourceAlphaBlendFactorInput.setCurrentText(pipelineData[29])
            view.destinationAlphaBlendFactorInput.setCurrentText(pipelineData[30])
            view.alphaBlendOperationInput.setCurrentText(pipelineData[31])
            view.logicOperationEnabledInput.setCurrentText(pipelineData[32])
            view.logicOperationInput.setCurrentText(pipelineData[33])
            view.attachmentCountInput.setValue(int(pipelineData[34]))
            view.blendConstant0Input.setValue(float(pipelineData[35].replace(',', '.')))
            view.blendConstant1Input.setValue(float(pipelineData[36].replace(',', '.')))
            view.blendConstant2Input.setValue(float(pipelineData[37].replace(',', '.')))
            view.blendConstant3Input.setValue(float(pipelineData[38].replace(',', '.')))
            view.vertexShaderFileInput.setText(pipelineData[39])
            view.vertexShaderEntryFunctionNameInput.setText(pipelineData[40])
            view.fragmentShaderFileInput.setText(pipelineData[41])
            view.fragmentShaderEntryFunctionNameInput.setText(pipelineData[42])

            view.addPipelineOKButton.accepted.connect(editPipeline)
            view.setWindowTitle("Edit Graphics Pipeline")
            view.vertexShaderFileToolButton.clicked.connect(
                lambda: self.setFilePath("vertexShaderFileToolButton", view))
            view.fragmentShaderFileToolButton.clicked.connect(
                lambda: self.setFilePath("fragmentShaderFileToolButton", view))
            view.exec_()

        selectedItem = self.graphicsPipelinesList.currentItem()
        if selectedItem is not None:
            updateGraphicsPipelineView(selectedItem)

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
        OKButton = QPushButton("OK")
        OKButton.clicked.connect(lambda: removePipelines(self.graphicsPipelinesList.selectedItems()))
        OKButton.clicked.connect(d.accept)
        layout.addWidget(OKButton)
        d.setLayout(layout)
        d.show()

    def fileOpenDialog(self):
        fileName = QFileDialog.getOpenFileName(self, 'Open file',
                                               'c:\\', "Model files (*.jpg)")

    def setFilePath(self, fileInput: str, referenceView):
        def setInputFilterBasedOnOrigin(fileInput):
            filterMap = {
                "modelFileToolButton": "Model files (*.obj)",
                "textureFileToolButton": "Texture image files (*.jpg *.jpeg *.png)",
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

    #################################
    ###### Validation section #######
    #################################

    def checkInput(self):
        missingInputs = []

        if len(self.applicationNameInput.text()) == 0:
            missingInputs.append((self.applicationNameLabel.text()).replace(":", ""))

        if not self.checkDeviceExtensions():
            return False

        if not self.checkPipelineInput():
            return False

        if missingInputs:
            self.showMissingInput(missingInputs)
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
        missingInputs = []

        for i in range(graphicsPipelines.count()):
            item = graphicsPipelines.item(i)
            data = item.data(Qt.UserRole)
            pipelineName = item.data(Qt.DisplayRole)

            vertexShaderFileInput = data[39]
            vertexShaderEntryFunctionNameInput = data[40]
            fragmentShaderFileInput = data[41]
            fragmentShaderEntryFunctionNameInput = data[42]

            if len(vertexShaderFileInput) == 0:
                missingInputs.append(f"{pipelineName}: Vertex Shader File")

            if len(fragmentShaderFileInput) == 0:
                missingInputs.append(f"{pipelineName}: Fragment Shader File")

            if len(vertexShaderEntryFunctionNameInput) == 0:
                missingInputs.append(f"{pipelineName}: Vertex Shader Entry Function Name")

            if len(fragmentShaderEntryFunctionNameInput) == 0:
                missingInputs.append(f"{pipelineName}: Fragment Shader Entry Function Name")

        if missingInputs:
            self.showMissingInput(missingInputs)
            return False

        return True

    #################################
    # Widget List and Items section #
    #################################

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
                return item

    #################################
    ##### XML In/Output section #####
    #################################
    def loadFromXMLFile(self):
        self.setFilePath("actionLoadFromFile", None)
        fileName = self.loadFile
        if fileName:
            self.readXML(fileName)

    def saveToXMLFile(self):
        # Show file dialog to select file storage location
        filePath, _ = QFileDialog.getSaveFileName(self, 'Save file', self.saveFile,
                                                  'Vulkan Setup GUI files (*.xml)')

        if filePath:
            self.writeXML(filePath)

    def writeXML(self, fileName):
        # create the file structure
        root = ET.Element('GraphicalVulkanEditor')

        # Instance
        instance = ET.SubElement(root, 'instance')
        ET.SubElement(instance, 'applicationNameInput',
                      name='applicationNameInput').text = \
            self.applicationNameInput.text()
        ET.SubElement(instance, 'showValidationLayerDebugInfoCheckBox',
                      name='showValidationLayerDebugInfoCheckBox').text = self.convertToVulkanNaming(
            self.showValidationLayerDebugInfoCheckBox.isChecked())
        ET.SubElement(instance, 'runOnMacosCheckBox', name='runOnMacosCheckBox').text = self.convertToVulkanNaming(
            self.runOnMacosCheckBox.isChecked())

        # Physical Device
        physicalDevice = ET.SubElement(root, 'physicalDevice')
        ET.SubElement(physicalDevice, 'chooseGPUOnStartupCheckBox',
                      name='chooseGPUOnStartupCheckBox').text = self.convertToVulkanNaming(
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
                      name='lockWindowSizeCheckBox').text = self.convertToVulkanNaming(
            self.lockWindowSizeCheckBox.isChecked())

        imageClearColor = ET.SubElement(swapchain, 'imageClearColor', name='imageClearColor')
        ET.SubElement(imageClearColor, 'clearColorRInput', name='clearColorRInput').text = self.clearColorRInput.text()
        ET.SubElement(imageClearColor, 'clearColorGInput', name='clearColorGInput').text = self.clearColorGInput.text()
        ET.SubElement(imageClearColor, 'clearColorBInput', name='clearColorBInput').text = self.clearColorBInput.text()
        ET.SubElement(imageClearColor, 'clearColorAInput', name='clearColorAInput').text = self.clearColorAInput.text()

        ET.SubElement(swapchain, 'framesInFlightInput',
                      name='framesInFlightInput').text = self.framesInFlightInput.text()
        ET.SubElement(swapchain, 'saveEnergyForMobileCheckBox',
                      name='saveEnergyForMobileCheckBox').text = self.convertToVulkanNaming(
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
        graphicsPipeline = ET.SubElement(root, 'graphicsPipeline')
        ET.SubElement(graphicsPipeline, 'useIndexedVerticesCheckBox', name='useIndexedVerticesCheckBox').text = self.convertToVulkanNaming(
            self.useIndexedVerticesCheckBox.isChecked())
        ET.SubElement(graphicsPipeline, 'reduceSpirvCodeSizeCheckBox', name='reduceSpirvCodeSizeCheckBox').text = self.convertToVulkanNaming(
            self.reduceSpirvCodeSizeCheckBox.isChecked())

        graphicsPipelines = ET.SubElement(graphicsPipeline, 'graphicsPipelines')

        for i in range(self.graphicsPipelinesList.count()):
            item = self.graphicsPipelinesList.item(i)
            pipeline = ET.SubElement(graphicsPipelines, 'pipeline', name=f'{item.data(Qt.DisplayRole)}')
            data = item.data(Qt.UserRole)

            pipelineInputs = [
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
                'fragmentShaderEntryFunctionNameInput'
            ]

            for index, value in enumerate(data):
                inputName = pipelineInputs[index]
                ET.SubElement(pipeline, inputName, name=inputName).text = str(value)

        # create a new XML file with the results
        mydata = ET.tostring(root)
        with open(fileName, "wb") as myfile:
            myfile.write(mydata)
        print(f"File created under: [{fileName}]")
        print("XML File created with parameters:")
        self.printInputs()

    def readXML(self, fileName):
        tree = ET.parse(fileName)
        root = tree.getroot()

        # recursively iterate over all elements and sub-elements
        def traverse(elem):
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
            if elem.tag == "useIndexedVerticesCheckBox":
                if elem.text == "VK_FALSE":
                    self.useIndexedVerticesCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
                    self.useIndexedVerticesCheckBox.setChecked(True)
            if elem.tag == "reduceSpirvCodeSizeCheckBox":
                if elem.text == "VK_FALSE":
                    self.reduceSpirvCodeSizeCheckBox.setChecked(False)
                if elem.text == "VK_TRUE":
                    self.reduceSpirvCodeSizeCheckBox.setChecked(True)

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

    #################################
    ### CPP Header Output section ###
    #################################
    def generatePipelineCode(self):
        """
        Prepares the graphics pipeline output by generating pipeline and shader code for each pipeline.

        Returns:
            dict: A dictionary mapping pipeline names to their respective pipeline and shader code.
        """

        def convertColorBlendAttachment_colorWriteMask(mask: str):
            if mask == "R":
                return "VK_COLOR_COMPONENT_R_BIT"
            elif mask == "G":
                return "VK_COLOR_COMPONENT_G_BIT"
            elif mask == "B":
                return "VK_COLOR_COMPONENT_B_BIT"
            elif mask == "A":
                return "VK_COLOR_COMPONENT_A_BIT"
            elif mask == "RG":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT"
            elif mask == "RB":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT"
            elif mask == "RA":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT"
            elif mask == "GB":
                return "VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT"
            elif mask == "GA":
                return "VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT"
            elif mask == "BA":
                return "VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"
            elif mask == "RGB":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT"
            elif mask == "RGA":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT"
            elif mask == "RBA":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"
            elif mask == "GBA":
                return "VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"
            elif mask == "RGBA":
                return "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"

        def appendFixedFunctionParameters(pipelineName: str, pipeline: list):
            params = f'''FixedFunctionStageParameters {pipelineName}{{
			//////////////////////// INPUT ASSEMBLY
			{pipeline[0]}, // inputAssemblyInfo_topology
			{pipeline[1]}, // inputAssemblyInfo_primitiveRestartEnable

			//////////////////////// RASTERIZER
			{pipeline[2]}, // rasterizerInfo_depthClampEnable
			{pipeline[3]}, // rasterizerInfo_rasterizerDiscardEnable
			{pipeline[4]}, // rasterizerInfo_polygonMode 
			{pipeline[5].replace(",", ".")}f, // rasterizerInfo_lineWidth
			{pipeline[6]}, // rasterizerInfo_cullMode
			{pipeline[7]}, // rasterizerInfo_frontFace
			{pipeline[8]}, // rasterizerInfodepthBiasEnable
			{pipeline[9].replace(",", ".")}f, // rasterizerInfo_depthBiasConstantFactor
			{pipeline[10].replace(",", ".")}f, // rasterizerInfo_depthBiasClamp
			{pipeline[11].replace(",", ".")}f, // rasterizerInfo_depthBiasSlopeFactor

			//////////////////////// DEPTH AND STENCIL
			{pipeline[12]}, // depthStencilInfo_depthTestEnable
			{pipeline[13]}, // depthStencilInfo_depthWriteEnable
			{pipeline[14]}, // depthStencilInfo_depthCompareOp
			{pipeline[15]}, // depthStencilInfo_depthBoundsTestEnable
			{pipeline[16].replace(",", ".")}f, // depthStencilInfo_minDepthBounds
			{pipeline[17].replace(",", ".")}f, // depthStencilInfo_maxDepthBounds
			{pipeline[18]}, // depthStencilInfo_stencilTestEnable

			//////////////////////// MULTISAMPLING
			{pipeline[19]}, // multisamplingInfo_sampleShadingEnable
			{pipeline[20]}, // multisamplingInfo_rasterizationSamples
			{pipeline[21].replace(",", ".")}f, // multisamplingInfo_minSampleShading
			{pipeline[22]}, // multisamplingInfo_alphaToCoverageEnable
			{pipeline[23]}, // multisamplingInfo_alphaToOneEnable

			//////////////////////// COLOR BLENDING
			{convertColorBlendAttachment_colorWriteMask(pipeline[24])}, // colorBlendAttachment_colorWriteMask
			{pipeline[25]}, // colorBlendAttachment_blendEnable
			{pipeline[26]}, // colorBlendAttachment_srcColorBlendFactor
			{pipeline[27]}, // colorBlendAttachment_dstColorBlendFactor
			{pipeline[28]}, // colorBlendAttachment_colorBlendOp
			{pipeline[29]}, // colorBlendAttachment_srcAlphaBlendFactor
			{pipeline[30]}, // colorBlendAttachment_dstAlphaBlendFactor
			{pipeline[31]}, // colorBlendAttachment_alphaBlendOp

			{pipeline[32]}, // colorBlendingInfo_logicOpEnable
			{pipeline[33]}, // colorBlendingInfo_logicOp
			{pipeline[34]}, // colorBlendingInfo_attachmentCount
			{pipeline[35].replace(",", ".")}f, // colorBlendingInfo_blendConstants_0
			{pipeline[36].replace(",", ".")}f, // colorBlendingInfo_blendConstants_1
			{pipeline[37].replace(",", ".")}f, // colorBlendingInfo_blendConstants_2
			{pipeline[38].replace(",", ".")}f // colorBlendingInfo_blendConstants_3
		}};\n
        '''
            return params

        def appendShaderStageParameters(pipelineName: str, pipeline: list):
            shaders = f'''ShaderStageParameters {pipelineName + "_shaders"}{{
			 "{pipeline[39]}", // vertexShaderText
			 "{pipeline[41]}", // fragmentShaderText
			 "{pipeline[40]}", // vertexShaderEntryFunctionName
			 "{pipeline[42]}" // fragmentShaderEntryFunctionName
		}};
        '''
            return shaders

        pipelineDict = {}
        for i in range(self.graphicsPipelinesList.count()):
            pipeline = self.graphicsPipelinesList.item(i).data(Qt.UserRole)
            pipelineName = self.graphicsPipelinesList.item(i).data(Qt.DisplayRole).lower().replace(" ", "_")
            pipelineCode = appendFixedFunctionParameters(pipelineName, pipeline)
            shaderCode = appendShaderStageParameters(pipelineName, pipeline)
            pipelineDict[pipelineName] = (pipelineCode, shaderCode)

        return pipelineDict

    def writeVulkanHeader(self):
        """
        Generates the Vulkan header file containing all the changeable but constant variables.

        This function generates the content for the VulkanProjectVariables.h header file.

        Returns:
            None
        """
        pipelineCodeDictionary = self.generatePipelineCode()
        pipelineNames = list(pipelineCodeDictionary.keys())
        shaderNames = [name + "_shaders" for name in pipelineNames]
        pipelineCode = ""
        shaderCode = ""

        for code in pipelineCodeDictionary.values():
            pipelineCode += code[0]
            shaderCode += code[1]

        #self.prepareGraphicsPipelineOutput()
        headerContent = f'''// This header includes all changeable but constant variables for the VulkanProject Header.
// Any changes should be made inside this header-file such that the original implementation can be kept untouched.
// DO NOT TOUCH THIS FILE. Any changes will be overridden on next save of Graphical Vulkan Editor.


#pragma once

namespace GVEProject {{

	// Instance
	const char* APPLICATION_NAME = "{self.applicationNameInput.text()}";
	const bool SHOW_VALIDATION_LAYER_DEBUG_INFO = {self.convertToVulkanNaming(self.showValidationLayerDebugInfoCheckBox.isChecked())};
	const bool RUN_ON_MACOS = {self.convertToVulkanNaming(self.runOnMacosCheckBox.isChecked())};

	// Physical Device
	const bool CHOOSE_GPU_ON_STARTUP = {self.convertToVulkanNaming(self.chooseGPUOnStartupCheckBox.isChecked())};

	// Device

	const std::vector<const char*> DEVICE_EXTENSIONS {{\n\t {
        ','.join(self.getListContents(self.deviceExtensionsList)[::-1])
	}\n\t}};

	// Swapchain
	const uint32_t WIDTH = {self.imageHeightInput.text()};
	const uint32_t HEIGHT = {self.imageWidthInput.text()};
	VkClearColorValue CLEAR_COLOR = {{ {{{self.clearColorRInput.text().replace(",", ".")}f, {self.clearColorGInput.text().replace(",", ".")}f, {self.clearColorBInput.text().replace(",", ".")}f, {self.clearColorAInput.text().replace(",", ".")}f}} }};
	const int MAX_FRAMES_IN_FLIGHT = {self.framesInFlightInput.text()};
	const bool LOCK_WINDOW_SIZE = {self.convertToVulkanNaming(self.lockWindowSizeCheckBox.isChecked())};
	const VkImageUsageFlagBits IMAGE_USAGE = {self.imageUsageInput.currentText()};
	const VkPresentModeKHR PRESENTATION_MODE = {self.presentationModeInput.currentText()};
	const bool SAVE_ENERGY_FOR_MOBILE = {self.convertToVulkanNaming(self.saveEnergyForMobileCheckBox.isChecked())};
	const VkFormat IMAGE_FORMAT = {self.imageFormatInput.currentText()};
	const VkColorSpaceKHR IMAGE_COLOR_SPACE = {self.imageColorSpaceInput.currentText()};
	
	// Descriptor

	// Shader

	// Model
	const std::string MODEL_FILE = "{self.modelFileInput.text()}";
	const std::string TEXTURE_FILE = "{self.textureFileInput.text()}";

	// Graphics Pipeline
    const bool USE_INDEXED_VERTICES = {self.convertToVulkanNaming(self.useIndexedVerticesCheckBox.isChecked())};
	const bool REDUCE_SPIRV_CODE_SIZE = {self.convertToVulkanNaming(self.reduceSpirvCodeSizeCheckBox.isChecked())};

		// Pipeline
		struct FixedFunctionStageParameters {{

			//////////////////////// INPUT ASSEMBLY
			const VkPrimitiveTopology inputAssemblyInfo_topology;
			const VkBool32 inputAssemblyInfo_primitiveRestartEnable;

			//////////////////////// RASTERIZER
			const VkBool32 rasterizerInfo_depthClampEnable; // clamp instead of discard fragments to far or near plane if are beyond, useful for e.g. shadow maps
			const VkBool32 rasterizerInfo_rasterizerDiscardEnable; // discard geometry passing through rasterizer, disables output to framebuffer.
			const VkPolygonMode rasterizerInfo_polygonMode;
			const float rasterizerInfo_lineWidth; // thickness of lines in terms of number of fragments
			const VkCullModeFlagBits rasterizerInfo_cullMode; //specify cull mode such as front-, back- or front-and-back culling
			const VkFrontFace rasterizerInfo_frontFace; // use counter clockwise to correct reversed draw oder caused by y-flip
			const VkBool32 rasterizerInfo_depthBiasEnable; // bias depth by adding a constant value, e.g. for shadow maps
			const float rasterizerInfo_depthBiasConstantFactor; // Optional
			const float rasterizerInfo_depthBiasClamp; // Optional
			const float rasterizerInfo_depthBiasSlopeFactor; // Optional

			//////////////////////// DEPTH AND STENCIL
			const VkBool32 depthStencilInfo_depthTestEnable; // specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
			const VkBool32 depthStencilInfo_depthWriteEnable; // specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
			const VkCompareOp depthStencilInfo_depthCompareOp; // specifies the comparison that is performed to keep or discard fragments. Use convention of lower depth = closer
			const VkBool32 depthStencilInfo_depthBoundsTestEnable;
			const float depthStencilInfo_minDepthBounds; // Optional, used for the optional depth bound test. Basically, this allows to only keep fragments that fall within the specified depth range. 
			const float depthStencilInfo_maxDepthBounds; // Optional, used for the optional depth bound test. Basically, this allows to only keep fragments that fall within the specified depth range. 
			const VkBool32 depthStencilInfo_stencilTestEnable;

			//////////////////////// MULTISAMPLING
			const VkBool32 multisamplingInfo_sampleShadingEnable;
			const VkSampleCountFlagBits multisamplingInfo_rasterizationSamples;
			const float multisamplingInfo_minSampleShading; // Optional
			const VkBool32 multisamplingInfo_alphaToCoverageEnable; // Optional
			const VkBool32 multisamplingInfo_alphaToOneEnable; // Optional

			//////////////////////// COLOR BLENDING
			const VkColorComponentFlags colorBlendAttachment_colorWriteMask;
			const VkBool32 colorBlendAttachment_blendEnable;
			const VkBlendFactor colorBlendAttachment_srcColorBlendFactor; // Optional
			const VkBlendFactor colorBlendAttachment_dstColorBlendFactor; // Optional
			const VkBlendOp colorBlendAttachment_colorBlendOp; // Optional
			const VkBlendFactor colorBlendAttachment_srcAlphaBlendFactor; // Optional
			const VkBlendFactor colorBlendAttachment_dstAlphaBlendFactor; // Optional
			const VkBlendOp colorBlendAttachment_alphaBlendOp; // Optional

			const VkBool32 colorBlendingInfo_logicOpEnable; //false applies to ALL attached framebuffers. Set to true if using e.g. alpha blending
			const VkLogicOp colorBlendingInfo_logicOp; // Optional
			const uint32_t colorBlendingInfo_attachmentCount;
			const float colorBlendingInfo_blendConstants_0; // Optional
			const float colorBlendingInfo_blendConstants_1; // Optional
			const float colorBlendingInfo_blendConstants_2; // Optional
			const float colorBlendingInfo_blendConstants_3; // Optional
		}};
		struct ShaderStageParameters {{
			const std::string vertexShaderText;
			const std::string fragmentShaderText;
			const char* vertexShaderEntryFunctionName; // choose entry point function within vertex shader
			const char* fragmentShaderEntryFunctionName; // choose entry point function within fragment shader
		}};
		
		// Functional Parameters 
		{pipelineCode}
		// Shader Parameters
		{shaderCode}

		const std::vector<FixedFunctionStageParameters> PIPELINE_PARAMETERS{{ {','.join(pipelineNames)} }};

		const std::vector<ShaderStageParameters> PIPELINE_SHADERS{{ {', '.join(shaderNames)} }};

		const int PIPELINE_COUNT = PIPELINE_PARAMETERS.size();


	// To be implemented
	const bool MIPMAP_LEVEL = 0;
	const VkBool32 ENABLE_ANISOTRIPIC_FILTER = VK_TRUE;

}}
        '''

        try:
            with open(VULKAN_HEADER_OUTPUT_LOCATION, "w") as myfile:
                myfile.write(headerContent)
            return True
        except IOError:
            return False

def main():
    app = QApplication([])
    window = GraphicalVulkanEditor()
    window.show()
    app.exec_()


if __name__ == '__main__':
    main()