#include "Scene.h"

#include "EngineGetter.h"
#include "LoggerGetter.h"
#include <format>


namespace Const
{
	constexpr SDL_Rect DefaultRect{ 500, 200, 100, 100 };
	constexpr ImColor DefaultColor{ 120, 200, 120, 255 };
	constexpr SDL_Point NewEntityOffset{ 50, 50 };
}

Scene::Scene() {
	_backgroundColor = ImColor(0.45f, 0.55f, 0.60f, 1.00f);

	initializeFrameBuffer(); // This is used to start the FrameBuffer on the scene, obviously.
}

void Scene::addEntity(Entity entity) {
	_pendingEntities.push(std::make_unique<Entity>(std::move(entity)));
}

void Scene::cloneEntity(const std::unique_ptr<Entity>& entity) {
	Entity clone = entity->clone(consumeId());
	clone.addPosition(Const::NewEntityOffset);
	addEntity(clone);
}

void Scene::update() {
	std::erase_if(_entities, [](const auto& entity) { return entity->getRemoved(); });

	while (!_pendingEntities.empty()) {
		_entities.push_back(std::move(_pendingEntities.front()));
		_pendingEntities.pop();
	}

	for (const auto& entity : _entities) {
		entity->update();
	}
}

void Scene::render() const {
	const auto& renderer = Engine::Get().getRenderer();

	const ImGuiIO& io = ImGui::GetIO();
	SDL_RenderSetScale(renderer.get(), io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

	renderer.setColor(_backgroundColor);
	SDL_RenderClear(renderer.get());

	SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND); // This enables alpha, which is used for transparency.
	for (const auto& entity : _entities) {
		entity->render();
	}
}

void Scene::renderImGui() {
	ImGui::ColorEdit3("Background", &_backgroundColor.Value.x);

	auto [x, y] = Engine::Get().getWindow().getSize();
	const auto text = std::format("Scene width and height: {} x {}", x, y);
	ImGui::Text("%s", text.c_str());

	ImGui::Separator();
	if (!ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) return;

	const auto cloneColor = static_cast<ImVec4>(ImColor{ 25, 100, 25, 255 });
	const auto removeColor = static_cast<ImVec4>(ImColor{ 128, 30, 10, 255 });

	for (const auto& entity : _entities) {
		std::string nameId = entity->getNameId();

		const bool showEntity = ImGui::TreeNode(nameId.c_str());

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, cloneColor);
		ImGui::SmallButton("Clone");
		if (ImGui::IsItemClicked()) {
			Log::Debug(std::format("{} Clone Button Clicked", nameId));
			cloneEntity(entity);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, removeColor);
		ImGui::SmallButton("Remove");
		if (ImGui::IsItemClicked()) {
			Log::Debug(std::format("{} Remove Button Clicked", nameId));
			entity->remove();
		}
		ImGui::PopStyleColor();

		if (showEntity) {
			entity->renderImGui();
			ImGui::TreePop();
		}
	}
}

void Scene::onWindowShown(const int width, const int height) {
	resetFrameBuffer(width, height); // This resets our FrameBuffer when the window is being displayed!!
}

// Also this resets the FrameBuffer when we resize the window so it doesnt crash no matter what size we do.
void Scene::onWindowResized(const int width, const int height) {
	resetFrameBuffer(width, height);
}

void Scene::resetFrameBuffer(const int width, const int height) {
	_entities.clear(); // This clears the entities everytime the FrameBuffer is being reset.
	_uniqueId = 0; // And this is used to recount the ID after we resize the window. 

	const int pixelSize = height / 16; // This is used to reload the size of every pixel no matter the size of the window.
	const int pixelRowSum = pixelSize * 16; // This is load the size of our FrameBuffer by using pixels. 

	SDL_Rect frameBufferRect{
		.x = (width - pixelSize * 16) / 2, // This is to place the center of our FrameBuffer horizontally.
		.y = (height - pixelSize * 16) / 2, // This is to place the center of our FrameBuffer vertically.
		.w = pixelSize * 16,
		.h = pixelSize * 16,
	};

	for (int i = 0; i < _frameBuffer.size(); ++i) { 
		const SDL_Point pixelCoords{ i % 16 , i / 16 }; // This calculates the pixel's grid coordinates which are X, and Y, obviously.

		const SDL_Point pixelPosition{
			.x = frameBufferRect.x + pixelCoords.x * pixelSize,
			.y = frameBufferRect.y + pixelCoords.y * pixelSize,
		};

		const SDL_Rect pixelRect = {
			.x = pixelPosition.x,
			.y = pixelPosition.y,
			.w = pixelSize,
			.h = pixelSize,
		};

		addEntity(Entity(consumeId(), "Pixel", pixelRect, _frameBuffer.at(i))); // This creates new pixels from the FrameBuffer.
	}

	Entity frameBufferEntity(consumeId(), "FrameBuffer", frameBufferRect, ImColor{ 255,255,255,255 });
	frameBufferEntity.setFilled(false);
	addEntity(std::move(frameBufferEntity));
}

void Scene::initializeFrameBuffer() {
	ImColor Void = { 255,255,255,0 }; // Void means transparent since we have alpha.
	ImColor Black = { 0,0,0,255 }; // This is the color black.
	ImColor White = { 255,255,255,255 }; // This is the color white.
	ImColor Green = { 28,148,134,255 }; // This is the color green.
	ImColor Red = { 206,52,52,255 }; // This is the color red.

	_frameBuffer = {
		Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,
		Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Black,Black,Void,Void,
		Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Black,Black,Green,Green,Black,Void,
		Void,Void,Void,Void,Void,Void,Void,Void,Black,Black,Green,Green,Green,Green,Black,Void,
		Void,Void,Void,Void,Void,Void,Void,Black,Green,Green,Black,Green,Black,Black,Void,Void,
		Void,Void,Void,Black,Black,Black,Black,Green,Black,Black,Black,Green,Black,Void,Void,Void,
		Void,Void,Black,Red,Red,Red,Red,Black,Black,Black,Green,Black,Void,Void,Void,Void,
		Void,Black,Red,Red,Red,Red,Red,Red,Black,Green,Black,Black,Void,Void,Void,Void,
		Void,Black,Red,Red,Red,Red,Red,Black,Red,Red,Red,Red,Black,Void,Void,Void,
		Void,Black,Red,White,Red,Red,Black,Red,Red,Red,Red,Red,Red,Black,Void,Void,
		Void,Black,Red,Red,White,White,Black,Red,Red,Red,Red,Red,Red,Black,Void,Void,
		Void,Void,Black,Red,Red,Red,Black,Red,White,Red,Red,Red,Red,Black,Void,Void,
		Void,Void,Void,Black,Black,Black,Black,Red,Red,White,White,Red,Red,Black,Void,Void,
		Void,Void,Void,Void,Void,Void,Void,Black,Red,Red,Red,Red,Black,Void,Void,Void,
		Void,Void,Void,Void,Void,Void,Void,Void,Black,Black,Black,Black,Void,Void,Void,Void,
		Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,Void,
	};
}

int Scene::consumeId() {
	return _uniqueId++;
}