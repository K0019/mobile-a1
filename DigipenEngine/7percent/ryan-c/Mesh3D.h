#pragma once

/******************************************************************************/
/*!
\file   Mesh3D.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
contains the mesh data structures for 3D and 2D meshes. 3D meshes do not actually exist in this project but the structure is here for future use. (maybe)

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Buffer.h"

struct VertexInputDescription {

	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
		glm::vec2 uv;
		static VertexInputDescription get_vertex_description();
};

struct Mesh3D {
	std::vector<Vertex> _vertices;

	AllocatedBuffer _vertexBuffer;
};

struct Vertex2D {
    glm::vec2 position;
    glm::vec2 texCoord;

    static VertexInputDescription getVertexDescription();
};

struct Mesh2D {
    std::vector<Vertex2D> vertices;
    AllocatedBuffer vertexBuffer;
};

struct LineInstanceData {
    glm::vec2 start;
    glm::vec2 end;
    glm::vec4 color;

  static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};