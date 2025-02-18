#pragma once

#include "InputBase.h"

namespace Pyxis
{
	namespace UI
	{
		/// <summary>
		/// Similar to how a button must be de-serialized separately from children,
		/// the input float must as well.
		/// </summary>
		class InputFloat : public InputBase
		{
		protected:

			float* m_Value;
			bool m_OwnsValue = false;

			std::string m_Text;

		public:
			//value input and params
			double m_MinValue = FLT_MIN;
			double m_MaxValue = FLT_MAX;

			//Text Display Variables
			glm::vec4 m_TextColor = glm::vec4(0, 0, 0, 1);
			glm::vec2 m_TextBorderSize = glm::vec2(0, 0);
			float m_FontSize = 1;
			Ref<Font> m_Font;
			bool m_Multiline = false;
			/// <summary>
			/// only works if multiline is false! (atm?)
			/// </summary>
			bool m_ScaleToFit = true;
			UI::Direction m_Alignment = UI::Direction::Left;


		public:
			InputFloat(const std::string& name, Ref<Font> font, float* value) : InputBase(name), 
				m_Font(font), m_Value(value)
			{
				if (value == nullptr)
				{
					m_OwnsValue = true;
					m_Value = new float();
					m_Text = std::to_string(*m_Value);
				}
				else
				{
					m_Value = value;
					m_Text = std::to_string(*m_Value);
				}
			}

			InputFloat(UUID id) : InputBase(id)
			{
				//defaults
				m_Value = new float();
				m_Text = std::to_string(*m_Value);				
				m_Font = ResourceManager::Load<Font>("assets/fonts/Aseprite.ttf");				
			}

			void SetValue(float* f)
			{
				if (f == nullptr) return;
				if (m_OwnsValue) delete m_Value;
				m_Value = f;
				m_Text = std::to_string(*m_Value);
				m_OwnsValue = false;
			}

			//Serialize
			virtual void Serialize(json& j) override
			{
				InputBase::Serialize(j);
				j["Type"] = "InputFloat";


				j["m_MinValue"] = m_MinValue;
				j["m_MaxValue"] = m_MaxValue;
				j["m_TextColor"] = m_TextColor;
				j["m_TextBorderSize"] = m_TextBorderSize;
				j["m_FontSize"] = m_FontSize;
				j["m_Font"] = m_Font->GetPath();
				j["m_Multiline"] = m_Multiline;
				j["m_ScaleToFit"] = m_ScaleToFit;
				j["m_Alignment"] = (int)m_Alignment;
			}
			//Deserialize
			virtual void Deserialize(json& j) override
			{
				InputBase::Deserialize(j);

				if (j.contains("m_MinValue")) j.at("m_MinValue").get_to(m_MinValue);
				if (j.contains("m_MaxValue")) j.at("m_MaxValue").get_to(m_MaxValue);
				if (j.contains("m_TextColor")) j.at("m_TextColor").get_to(m_TextColor);
				if (j.contains("m_TextBorderSize")) j.at("m_TextBorderSize").get_to(m_TextBorderSize);
				if (j.contains("m_FontSize")) j.at("m_FontSize").get_to(m_FontSize);
				if (j.contains("m_Font")) m_Font = ResourceManager::Load<Font>(j.at("m_Font").get<std::string>());
				if (j.contains("m_Multiline")) j.at("m_Multiline").get_to(m_Multiline);
				if (j.contains("m_ScaleToFit")) j.at("m_ScaleToFit").get_to(m_ScaleToFit);
				if (j.contains("m_Alignment")) j.at("m_Alignment").get_to(m_Alignment);
			}


			~InputFloat()
			{
				if (m_OwnsValue) delete m_Value;
			}

			virtual void OnInspectorRender() override
			{
				InputBase::OnInspectorRender();

				if (ImGui::TreeNodeEx("Int Edit", ImGuiTreeNodeFlags_DefaultOpen))
				{
					//float things
					ImGui::InputDouble("Min Value", &m_MinValue);
					ImGui::InputDouble("Max Value", &m_MaxValue);

					//text things
					ImGui::ColorEdit4("Text Color", glm::value_ptr(m_TextColor));
					ImGui::DragFloat("Font Size", &m_FontSize);
					ImGui::Checkbox("Multiline", &m_Multiline);
					ImGui::InputFloat2("Text Border Size", glm::value_ptr(m_TextBorderSize));
					ImGui::Checkbox("Scale To Fit", &m_ScaleToFit);
					if (ImGui::TreeNodeEx("Alignment", ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (ImGui::Button("Left")) { m_Alignment = Direction::Left; }
						if (ImGui::Button("Centered")) { m_Alignment = Direction::Center; }
						//if (ImGui::Button("Right")) { m_Alignment = Direction::Right; }
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

			virtual void OnKeyTyped(KeyTypedEvent& e) override
			{
				if (m_Enabled && s_MousePressedNodeID == m_UUID)
				{
					int keycode = e.GetKeyCode();

					m_Text.push_back(keycode);
					int newValue = std::max(std::min(std::atof(m_Text.c_str()), m_MaxValue), m_MinValue);
					m_Text = std::to_string(newValue);
					*m_Value = newValue;
				}

			}

			virtual void OnKeyPressed(KeyPressedEvent& e) override
			{
				if (m_Enabled && s_MousePressedNodeID == m_UUID)
				{
					int keycode = e.GetKeyCode();

					if (keycode == PX_KEY_BACKSPACE)
					{
						if (m_Text.size() > 0)
							m_Text.pop_back();
					}

					int newValue = std::max(std::min(std::atof(m_Text.c_str()), m_MaxValue), m_MinValue);

					if (keycode == PX_KEY_MINUS)
					{
						newValue *= -1;
					}
					m_Text = std::to_string(newValue);
					*m_Value = newValue;
				}

			}

			virtual void OnRender() override
			{
				InputBase::OnRender();

				if (m_Enabled)
				{

					float characterHeight = m_FontSize * m_Font->m_CharacterHeight * 1.3f;

					glm::vec2 maxSize = m_Size - (m_TextBorderSize * (2.0f / m_PPU));
					if (m_Multiline)
					{
						Renderer2D::DrawText(m_Text, GetWorldTransform() * glm::translate(glm::mat4(1), { -m_Size.x / 2, (m_Size.y / 2) - characterHeight, -0.0001f }), m_Font, m_FontSize, 1.3f, m_Size.x - m_TextBorderSize.x, m_Alignment, m_TextColor, m_UUID);
					}
					else
					{
						//see if we want to add an input indicator
						auto time = std::chrono::high_resolution_clock::now();
						long long ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time).time_since_epoch().count();

						const char* indicator = (s_MousePressedNodeID == m_UUID) && (ms / 500) % 2 == 0 ? "|" : "";

						Renderer2D::DrawTextLine(m_Text + indicator, GetWorldTransform() * glm::translate(glm::mat4(1), { 0, 0, -0.0001f }), m_Font, maxSize, m_FontSize, m_Alignment, m_ScaleToFit, m_TextColor, m_UUID);
					}
				}
			}

		};
		REGISTER_SERIALIZABLE_NODE(InputFloat);
	}
}
