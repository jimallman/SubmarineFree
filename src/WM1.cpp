#include <settings.hpp>
#include "SubmarineFree.hpp"

struct EventButton : OpaqueWidget {
	std::function<void ()> clickHandler;
	std::function<void ()> rightClickHandler;
	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			if (clickHandler) {
				clickHandler();
				e.consume(this);
				return;
			}
		}
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			if (rightClickHandler) {
				rightClickHandler();
				e.consume(this);
				return;
			}
		}
		OpaqueWidget::onButton(e);
	}
};

struct SizeableModuleWidget : SchemeModuleWidget {
	bool stabilized = false;
	float fullSize = 0;
	SchemePanel *panel;
	SizeableModuleWidget(Module *module, float size) : SchemeModuleWidget(module) {
		fullSize = size;
		this->box.size = Vec(fullSize, 380);
		panel = new SchemePanel(this->box.size);
		addChild(panel);
	}
	void Resize() {
		panel->box.size = this->box.size;
		panel->dirty = true;
		onResize();
	}
	void Minimize(bool minimize) {
		float oldSize = box.size.x;
		box.size.x = minimize?15:fullSize;
		ShiftOthers(box.size.x - oldSize);
		Resize();
	}
	void ShiftOthers(float delta) {
		if (!stabilized)
			return;
		if (delta == 0.0f)
			return;
		for (Widget *w : APP->scene->rack->moduleContainer->children) {
			if (this == w)
				continue;
			if (this->box.pos.x > w->box.pos.x) 
				continue;
			if (this->box.pos.y != w->box.pos.y)
				continue;
			w->box.pos.x += delta;
		}
	}
	json_t *toJson() override {
		json_t *rootJ = ModuleWidget::toJson();
		json_object_set_new (rootJ, "width", json_real(box.size.x));
		return rootJ;
	}
	void fromJson(json_t *rootJ) override {
		ModuleWidget::fromJson(rootJ);
		json_t *widthJ = json_object_get(rootJ, "width");
		if (widthJ)
			box.size.x = json_number_value(widthJ);
		Minimize(box.size.x < 16.0f);
	}
	virtual void onResize() {
	}
};

struct MinButton : EventButton {
	SizeableModuleWidget *mw;
	MinButton() {
		this->box.size = Vec(10, 20);
		this->clickHandler = [=]() {
			mw->Minimize(mw->box.size.x > 16.0f);
		};
	}
	void draw(const DrawArgs &args) override {
		nvgBeginPath(args.vg);
		if (this->box.pos.x < 15.0f) {
			nvgMoveTo(args.vg, 0, 0);
			nvgLineTo(args.vg, 10, 10);
			nvgLineTo(args.vg, 0, 20);
			nvgClosePath(args.vg);
		}
		else {
			nvgMoveTo(args.vg, 10, 0);
			nvgLineTo(args.vg, 0, 10);
			nvgLineTo(args.vg, 10, 20);
			nvgClosePath(args.vg);
		}
		nvgFillColor(args.vg, gScheme.getAlternative(mw->module));
		nvgFill(args.vg);
		Widget::draw(args);
	}
};

struct BackPanel : Widget {
	void draw(const DrawArgs &args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(args.vg, nvgRGB(0 ,0 ,0));
		nvgFill(args.vg);
		Widget::draw(args);
	}
};

struct EventSlider : OpaqueWidget {
	int transparent = false;
	float value = 0.5f;
	float minValue = 0.0f;
	float maxValue = 1.0f;
	std::function<void(float)> changeHandler;
	void draw(const DrawArgs &args) override {
		DEBUG("%f", value);
		Vec minHandlePos;
		Vec maxHandlePos;
		float width;
		float height;
		if (box.size.x < box.size.y) {
			width = box.size.x;
			height = 10;
			minHandlePos = Vec(box.size.x / 2, height / 2);
			maxHandlePos = Vec(box.size.x / 2, box.size.y - height / 2);
		}
		else {
			width = 10;
			height = box.size.y;
			minHandlePos = Vec(width / 2, box.size.y / 2);
			maxHandlePos = Vec(box.size.x - width / 2, box.size.y / 2);
		}
		Vec pos = Vec(rescale(value, minValue, maxValue, minHandlePos.x, maxHandlePos.x), rescale(value, minValue, maxValue, minHandlePos.y, maxHandlePos.y));
		nvgFillColor(args.vg, nvgRGB(0, 0, 0));
		nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgStrokeWidth(args.vg, 1);
		if (!transparent) {
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, minHandlePos.x, minHandlePos.y);
			nvgLineTo(args.vg, maxHandlePos.x, maxHandlePos.y);
			nvgStroke(args.vg);
		}
		nvgBeginPath(args.vg);
		nvgRect(args.vg, pos.x - width / 2 + 0.5, pos.y - height / 2 + 0.5, width - 1, height - 1);
		if (!transparent) nvgFill(args.vg);
		nvgStroke(args.vg);
	}
	void onDragStart(const event::DragStart &e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
		APP->window->cursorLock();
	}
	void onDragMove(const event::DragMove &e) override {
		float oldValue = value;
		value += e.mouseDelta.x * (maxValue - minValue) * 0.001f;
		value = clamp(value, minValue, maxValue);
		if (oldValue != value) {
			if (changeHandler) {
				changeHandler(value);
			}
		}
	}
	void onDragEnd(const event::DragEnd &e) override {
		APP->window->cursorUnlock();
	}

};

struct CheckBox : EventButton {
	std::string label;
	int selected = false;
	std::function<void()> changeHandler;
	CheckBox() {
		clickHandler = [=]() { 
			selected = !selected;
			if (changeHandler) {
				changeHandler();
			}
		};
	}
	void draw (const DrawArgs &args) override {
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		if (!label.empty()) {
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgFontSize(args.vg, 13);	
			nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE);
			nvgText(args.vg, 21, box.size.y / 2, label.c_str(), NULL);
		}
		nvgStrokeWidth(args.vg, 1);
		nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		if (selected) {
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, box.size.y / 2 - 4, box.size.y / 2 - 5);
			nvgLineTo(args.vg, box.size.y / 2 + 6, box.size.y / 2 + 5);
			nvgMoveTo(args.vg, box.size.y / 2 - 4, box.size.y / 2 + 5);
			nvgLineTo(args.vg, box.size.y / 2 + 6, box.size.y / 2 - 5);
			nvgStroke(args.vg);
		}
		nvgBeginPath(args.vg);
		nvgRect(args.vg, box.size.y / 2 - 7, box.size.y / 2 - 8, 16, 16);
		nvgStroke(args.vg);
		OpaqueWidget::draw(args);
	}
};

struct AddButton : EventButton {
	void draw (const DrawArgs &args) override {
		nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgStrokeWidth(args.vg, 2);
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, box.size.x / 2, 0);
		nvgLineTo(args.vg, box.size.x / 2, box.size.y);
		nvgMoveTo(args.vg, 0, box.size.y / 2);
		nvgLineTo(args.vg, box.size.x, box.size.y / 2);

		nvgStroke(args.vg);
		OpaqueWidget::draw(args);
	}
};

struct MenuButton : EventButton {
	void draw (const DrawArgs &args) override {
		nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgStrokeWidth(args.vg, 2);
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 0, box.size.y / 2 - 4);
		nvgLineTo(args.vg, box.size.x, box.size.y / 2 - 4);
		nvgMoveTo(args.vg, 0, box.size.y / 2);
		nvgLineTo(args.vg, box.size.x, box.size.y / 2);
		nvgMoveTo(args.vg, 0, box.size.y / 2 + 4);
		nvgLineTo(args.vg, box.size.x, box.size.y / 2 + 4);

		nvgStroke(args.vg);
		OpaqueWidget::draw(args);
	}
};

struct EventLabel : Widget {
	std::string label;
	void draw(const DrawArgs &args) override {
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		if (!label.empty()) {
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgFontSize(args.vg, 13);
			nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
			nvgText(args.vg, box.size.x / 2, box.size.y / 2, label.c_str(), NULL);
		}
		Widget::draw(args);
	}
};

struct RectButton : OpaqueWidget {
	std::string label;
	std::function<void ()> clickHandler;
	void draw(const DrawArgs &args) override {
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		if (!label.empty()) {
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgFontSize(args.vg, 13);
			nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
			nvgText(args.vg, box.size.x / 2, box.size.y / 2, label.c_str(), NULL);
		}
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.5, 0.5, box.size.x - 1, box.size.y - 1);
		nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgStrokeWidth(args.vg, 1);
		nvgStroke(args.vg);
		OpaqueWidget::draw(args);
	}
	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			e.consume(this);
			if (clickHandler) {
				clickHandler();
			}
			return;
		}
		OpaqueWidget::onButton(e);
	}
};

struct EditPanel : BackPanel {
	std::function<void(NVGcolor)> completeHandler;
	std::function<void()> cancelHandler;
	EventSlider *r;
	EventSlider *g;
	EventSlider *b;
	EditPanel() {
		r = new EventSlider();
		r->box.pos = Vec(10, 105);
		r->box.size = Vec(110, 19);
		addChild(r);
		g = new EventSlider();
		g->box.pos = Vec(10, 145);
		g->box.size = Vec(110, 19);
		addChild(g);
		b = new EventSlider();
		b->box.pos = Vec(10, 185);
		b->box.size = Vec(110, 19);
		addChild(b);
		RectButton *saveButton = new RectButton();
		saveButton->box.pos = Vec(5, 250);
		saveButton->box.size = Vec(55, 19);
		saveButton->label = "Save";
		saveButton->clickHandler = [=](){
			if (this->completeHandler) {
				this->completeHandler(nvgRGBf(r->value, g->value, b->value));
			}
		};
		addChild(saveButton);
		
		RectButton *cancelButton = new RectButton();
		cancelButton->box.pos = Vec(70, 250);
		cancelButton->box.size = Vec(55, 19);
		cancelButton->label = "Cancel";
		cancelButton->clickHandler = [=](){
			if (this->cancelHandler) {
				this->cancelHandler();
			}
		};
		addChild(cancelButton);
	}
	void drawBackground(NVGcontext *vg, float r1, float r2, float g1, float g2, float b1, float b2, float y) {
		NVGpaint grad = nvgLinearGradient(vg, r->box.pos.x + 5, 100, r->box.size.x - 10, 100, nvgRGBf(r1, g1, b1), nvgRGBf(r2, g2, b2));
		nvgBeginPath(vg);
		nvgFillPaint(vg, grad);
		nvgRect(vg, r->box.pos.x, y, r->box.size.x, 10);
		nvgFill(vg);
	}
	void draw(const DrawArgs &args) override {
		BackPanel::draw(args);
		NVGcolor color = nvgRGBf(r->value, g->value, b->value);
		NVGcolor colorOutline = nvgLerpRGBA(color, nvgRGBf(0.0, 0.0, 0.0), 0.5);
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 12, 12);
		nvgQuadTo(args.vg, box.size.x / 2, 150, box.size.x - 12, 12);
		nvgStrokeColor(args.vg, colorOutline);
		nvgStrokeWidth(args.vg, 5);
		nvgStroke(args.vg);

		nvgStrokeColor(args.vg, color);
		nvgStrokeWidth(args.vg, 3);
		nvgStroke(args.vg);

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, 12, 12, 9);
		nvgCircle(args.vg, box.size.x - 12, 12, 9);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);

		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, colorOutline);
		nvgStroke(args.vg);
	
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, 12, 12, 5);
		nvgCircle(args.vg, box.size.x - 12, 12, 5);
		nvgFillColor(args.vg, nvgRGB(0, 0, 0));
		nvgFill(args.vg);

		drawBackground(args.vg, 0, 1, g->value, g->value, b->value, b->value, 109.5f);	
		drawBackground(args.vg, r->value, r->value, 0, 1, b->value, b->value, 149.5f);
		drawBackground(args.vg, r->value, r->value, g->value, g->value, 0, 1, 189.5f);
	}
};

struct EventMenuItem : MenuItem {
	std::function<void()> clickHandler;
	void onAction(const event::Action &e) override {
		e.consume(this);
		if (clickHandler) {
			clickHandler();
		}
	}	
};

struct WireButton : EventButton {
	NVGcolor color;
	CheckBox *checkBox;
	WireButton() {
		checkBox = new CheckBox();
		checkBox->box.pos = Vec(1,1);
		checkBox->box.size = Vec(19, 19);
		addChild(checkBox);
	}
	void draw(const DrawArgs &args) override {
		NVGcolor colorOutline = nvgLerpRGBA(color, nvgRGBf(0.0, 0.0, 0.0), 0.5);
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 32, box.size.y / 2);
		nvgLineTo(args.vg, box.size.x, box.size.y / 2);
		nvgStrokeColor(args.vg, colorOutline);
		nvgStrokeWidth(args.vg, 5);
		nvgStroke(args.vg);
		
		nvgStrokeColor(args.vg, color);
		nvgStrokeWidth(args.vg, 3);
		nvgStroke(args.vg);

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, 32, box.size.y / 2, 9);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);
		
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, colorOutline);
		nvgStroke(args.vg);

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, 32, box.size.y / 2, 5);
		nvgFillColor(args.vg, nvgRGBf(0.0, 0.0, 0.0));
		nvgFill(args.vg);

		OpaqueWidget::draw(args);
	}
};

struct WM101 : SizeableModuleWidget {
	MinButton *minButton;
	BackPanel *backPanel;
	BackPanel *deleteConfirmPanel;
	BackPanel *settingsPanel;
	RectButton *deleteCancelButton;
	RectButton *deleteOkButton;
	EditPanel *editPanel;
	EventSlider *rSlider;
	
	ScrollWidget *scrollWidget;
	WM101(Module *module) : SizeableModuleWidget(module, 150) {
		minButton = new MinButton();
		minButton->box.pos = Vec(140,180);
		minButton->mw = this;
		addChild(minButton); 
		backPanel = new BackPanel();
		backPanel->box.pos = Vec(10, 15);
		backPanel->box.size = Vec(box.size.x - 20, box.size.y - 30);
		addChild(backPanel);
		scrollWidget = new ScrollWidget();
		scrollWidget->box.pos = Vec(0, 21);
		scrollWidget->box.size = Vec(backPanel->box.size.x, backPanel->box.size.y - 21);
		backPanel->addChild(scrollWidget);
		CheckBox *checkBoxAll = new CheckBox();
		checkBoxAll->box.pos = Vec(1,1);
		checkBoxAll->box.size = Vec(19, 19);
		checkBoxAll->changeHandler = [=](){
			this->checkAll(checkBoxAll->selected);
		};
		backPanel->addChild(checkBoxAll);
		AddButton *addButton = new AddButton();
		addButton->box.pos = Vec(57, 2);
		addButton->box.size = Vec(16, 16);
		addButton->clickHandler = [=]() {
			this->editDialog(NULL);
		};
		backPanel->addChild(addButton);
		MenuButton *menuButton = new MenuButton();
		menuButton->box.pos = Vec(112, 2);
		menuButton->box.size = Vec(16, 16);
		menuButton->clickHandler = [=]() {
			this->addMenu();
		};
		menuButton->rightClickHandler = menuButton->clickHandler;
		backPanel->addChild(menuButton);

		deleteConfirmPanel = new BackPanel();
		deleteConfirmPanel->box.pos = backPanel->box.pos;
		deleteConfirmPanel->box.size = backPanel->box.size;
		deleteConfirmPanel->visible = false;
		addChild(deleteConfirmPanel);

		EventLabel *deleteLabel = new EventLabel();
		deleteLabel->box.pos = Vec(15, 195);
		deleteLabel->box.size = Vec(box.size.x - 40, 19);
		deleteLabel->label = "Delete Color?";
		deleteConfirmPanel->addChild(deleteLabel);
		
		deleteOkButton = new RectButton();
		deleteOkButton->box.pos = Vec(5, 250);
		deleteOkButton->box.size = Vec(55, 19);
		deleteOkButton->label = "OK";
		deleteConfirmPanel->addChild(deleteOkButton);
		
		deleteCancelButton = new RectButton();
		deleteCancelButton->box.pos = Vec(70, 250);
		deleteCancelButton->box.size = Vec(55, 19);
		deleteCancelButton->label = "Cancel";
		deleteConfirmPanel->addChild(deleteCancelButton);

		editPanel = new EditPanel();
		editPanel->box.pos = backPanel->box.pos;
		editPanel->box.size = backPanel->box.size;
		editPanel->visible = false;
		editPanel->cancelHandler = [=]() {
			this->cancel();
		};
		addChild(editPanel);
		
		settingsPanel = new BackPanel();
		settingsPanel->box.pos = backPanel->box.pos;
		settingsPanel->box.size = backPanel->box.size;
		settingsPanel->visible = false;
		addChild(settingsPanel);
	
		RectButton *settingsButton = new RectButton();
		settingsButton->box.pos = Vec(5, 250);
		settingsButton->box.size = Vec(55, 19);
		settingsButton->label = "Close";
		settingsButton->clickHandler = [=]() {
			this->cancel();
		};
		settingsPanel->addChild(settingsButton);

		loadSettings();
	}
	void render(NVGcontext *vg, SchemeCanvasWidget *canvas) override {
		if (this->box.size.x > 16.0f) {
			drawBase(vg, "WM-101");
		}
		else {
			drawBackground(vg);
			drawLogo(vg, 0, 380, 1, -M_PI / 2.0f);
			nvgSave(vg);
			nvgTranslate(vg, 0, 377);
			nvgRotate(vg, -M_PI / 2.0f);
			drawText(vg, 20, 0, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, 12, gScheme.getAlternative(module), "submarine");
			drawText(vg, 220, 0, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, 12, gScheme.getAlternative(module), "WM-101 Wire Manager");
			nvgRestore(vg);
		}
	}
	void step() override {
		if (!stabilized) {
			stabilized = true;
		}
		SizeableModuleWidget::step();
	}
	void onResize() override {
		bool small = this->box.size.x < 16.0f;
		minButton->box.pos.x = small?2.5f:(this->box.size.x - 10.0f);
		backPanel->visible = !small;
		deleteConfirmPanel->visible = false;
		SizeableModuleWidget::onResize();	
	}
	void addColor(NVGcolor color, int selected) {
		float y = scrollWidget->container->children.size() * 21;
		WireButton *wb = new WireButton();
		wb->box.pos = Vec(0, y);
		wb->box.size = Vec(scrollWidget->box.size.x, 21);
		wb->color = color;
		wb->checkBox->selected = selected;
		wb->rightClickHandler = [=]() {
			this->addWireMenu(wb);
		};
		scrollWidget->container->addChild(wb);
	}
	void setDefaults() {
		for (NVGcolor color : settings::cableColors) {
			addColor(color, true);
		}
		
		addColor(nvgRGB(0xff, 0xae, 0xc9), false);
		addColor(nvgRGB(0xb7, 0x00, 0xb5), false);
		addColor(nvgRGB(0x80, 0x80, 0x80), false);
		addColor(nvgRGB(0xff, 0xff, 0xff), false);
		addColor(nvgRGB(0x10, 0x0f, 0x12), false);
		addColor(nvgRGB(0xff, 0x99, 0x41), false);
		addColor(nvgRGB(0x80, 0x36, 0x10), false);
	}
	void loadSettings() {
		setDefaults();
	}
	void checkAll(bool selected) {
		for (Widget *widget : scrollWidget->container->children) {
			WireButton *wb = dynamic_cast<WireButton *>(widget);
			wb->checkBox->selected = selected;
		}
	}
	bool isFirst(WireButton *wb) {
		return scrollWidget->container->children.front() == wb;
	}
	bool isLast(WireButton *wb) {
		return scrollWidget->container->children.back() == wb;
	}
	void addMenu() {
		Menu *menu = createMenu();
		EventMenuItem *add = new EventMenuItem();
		add->text = "Add new color";
		add->clickHandler = [=]() {
			this->editDialog(NULL);	
		};
		menu->addChild(add);

		EventMenuItem *settings = new EventMenuItem();
		settings->text = "Settings";
		settings->clickHandler = [=]() {
			this->settingsDialog();
		};
		menu->addChild(settings);
	}
	void addWireMenu(WireButton *wb) {
		Menu *menu = createMenu();
		MenuLabel *label = new MenuLabel();
		label->text = "Color: " + color::toHexString(wb->color);
		menu->addChild(label);
		EventMenuItem *ed = new EventMenuItem();
		ed->text = "Edit";
		ed->clickHandler = [=]() {
			this->editDialog(wb);
		};
		menu->addChild(ed);
		if (!isFirst(wb)) {
			EventMenuItem *mu = new EventMenuItem();
			mu->text = "Move Up";
			mu->clickHandler = [=]() {
				this->swap(wb->box.pos.y / 21 - 1);
			};
			menu->addChild(mu);
		}
		if (!isLast(wb)) {
			EventMenuItem *md = new EventMenuItem();
			md->text = "Move Down";
			md->clickHandler = [=]() {
				this->swap(wb->box.pos.y / 21);
			};
			menu->addChild(md);
		}
		EventMenuItem *dm = new EventMenuItem();
		dm->text = "Delete";
		dm->clickHandler = [=]() {
			this->deleteDialog(wb);
		};
		menu->addChild(dm);
	}
	void deleteWire(WireButton *wb) {
		scrollWidget->container->removeChild(wb);
		delete wb;
		reflow();
	}
	void reflow() {
		unsigned int y = 0;
		for (Widget *widget : scrollWidget->container->children) {
			WireButton *wb = dynamic_cast<WireButton *>(widget);
			wb->box.pos.y = y;
			y += 21;
		}
		
	}
	void swap(int i) {
		if (i < 0) return;
		if (i > (int)(scrollWidget->container->children.size()) - 2) return;
		auto vi = std::begin(scrollWidget->container->children);
		std::advance(vi, i);
		WireButton *wb1 = dynamic_cast<WireButton *>(* vi++);
		WireButton *wb2 = dynamic_cast<WireButton *>(* vi);
		NVGcolor col = wb1->color;
		wb1->color = wb2->color;
		wb2->color = col;
		bool sel = wb1->checkBox->selected;
		wb1->checkBox->selected = wb2->checkBox->selected;
		wb2->checkBox->selected = sel;
	}
	void deleteDialog(WireButton *wb) {
		deleteCancelButton->clickHandler = [=]() {
			this->cancel();	
		};	
		deleteOkButton->clickHandler = [=]() {
			this->deleteOk(wb);
		};
		backPanel->visible = false;
		deleteConfirmPanel->visible = true;
	}
	void cancel() {
		backPanel->visible = true;
		editPanel->visible = false;
		settingsPanel->visible = false;
		deleteConfirmPanel->visible = false;
	}
	void deleteOk(WireButton *wb) {
		this->deleteWire(wb);
		cancel();
	}
	void settingsDialog() {
		backPanel->visible = false;
		settingsPanel->visible = true;
	}
	void editDialog(WireButton *wb) {
		backPanel->visible = false;
		editPanel->completeHandler = [=](NVGcolor col) {
			if (wb) {
				wb->color = col;
			}
			else {
				addColor(col, false);
			}
			cancel();
		};
		if (wb) {
			editPanel->r->value = wb->color.r;
			editPanel->g->value = wb->color.g;
			editPanel->b->value = wb->color.b;
		}
		else {
			editPanel->r->value = 0.5f;
			editPanel->g->value = 0.5f;
			editPanel->b->value = 0.5f;
		}
		editPanel->visible = true;
	}
};

Model *modelWM101 = createModel<Module, WM101>("WM-101");
