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

	initializeFrameBuffer();
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

	SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
	for (const auto& entity : _entities) {
		entity->render();
	}

	SDL_RenderCopy(renderer.get(), _frameBuffer.get(), nullptr, nullptr);
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
	resetFrameBuffer(width, height);
}

void Scene::onWindowResized(const int width, const int height) {
	resetFrameBuffer(width, height);
}

void Scene::resetFrameBuffer(const int width, const int height) {
	//This creates the texture for the FrameBuffer.
	_frameBuffer = std::unique_ptr<SDL_Texture, TextureDestroyer>(
		SDL_CreateTexture(Engine::Get().getRenderer().get(),
			SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
			width, height));

	//What this does is access the pixel data, however needs to be locked.
	void* pixels;
	int pitch;
	SDL_LockTexture(_frameBuffer.get(), nullptr, &pixels, &pitch);
	const auto pixelData = static_cast<Uint32*>(pixels);

	Log::Debug(std::format("pitch: {}", pitch));

	const int pixelsPerRow = pitch / sizeof(Uint32); //"pixelsPerRow" calculates the number of pixels we have in a row, obviously.
	const int numDivisions = 10; //"numDivisions" defines the number of vertical blocks, which is 10, so we don't really need to keep typing the number 10.
	const int blockWidth = width / numDivisions; //This calculation is used to define the size of each vertical block, since the window's size can be changed without crashing. Devides the width of the screen with the number of divisions of the vertical blocks, aka 10.

	for (int i = 0; i < numDivisions; ++i) {
		//This is used to calculate the color red based of the block index.
		ImColor color{
			static_cast<int>((255.0f * i) / (numDivisions - 1)),
			0,
			0,
			255 //Defines the color red.
		};

		//This defines the X of the coordinates for this one block.
		int xStart = i * blockWidth;
		int xEnd = (i == numDivisions - 1) ? width : xStart + blockWidth; //Last block fills the one that remains.

		//This fills each block with the same color. The color depends on the block.
		for (int y = 0; y < height; ++y) {
			for (int x = xStart; x < xEnd; ++x) {
				const int index = y * pixelsPerRow + x;
				pixelData[index] = static_cast<ImU32>(color);
			}
		}
	}

	//Unlocks the texture.
	SDL_UnlockTexture(_frameBuffer.get());
}

int Scene::consumeId() {
	return _uniqueId++;
}

void Scene::initializeFrameBuffer() {}

void Scene::TextureDestroyer::operator()(SDL_Texture* texture) const {
	SDL_DestroyTexture(texture);
}
