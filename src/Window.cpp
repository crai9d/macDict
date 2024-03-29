#include "Window.h"
#include "LineEdit.h"
#include "Dictionary.h"
#include <QtWebEngine/QtWebEngine>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtCore/QSignalBlocker>
#include <sstream>

static QPushButton *add_flat_btn(
	const char * const text,
	QWidget * const parent
) {
	QPushButton * const btn = new QPushButton(text, parent);
	btn->setFlat(true);
	btn->setFixedSize(QSize(35, 30));
	return btn;
}

Window::Window(
	const DictionaryRef &dict,
	const bool dark,
	const std::string &word,
	QWidget *parent
) : QMainWindow(parent),
    _dict(dict),
    _dark(dark)
{
	setWindowTitle("Dictionary");

	_split	   = new QSplitter(Qt::Horizontal, this);
	_left	   = new QWidget(_split);
	_right	   = new QWidget(_split);
	_top_left  = new QWidget(_left);
	_top_right = new QWidget(_right);
	_line	   = new LineEdit(word, _top_right);
	_scroll	   = new QScrollArea(_right);

	_view	= new QWebEngineView(_scroll);
	_view->setZoomFactor(1.25);

	QWebEngineProfile::defaultProfile()->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);


	_theme  = new QPushButton("Theme", _top_left);
	_found	= new QLabel("", _top_right);
	// _back  = add_flat_btn("<", _top_right);
	// _fwd   = add_flat_btn(">", _top_right);
	_small = add_flat_btn("-", _top_right);
	_big   = add_flat_btn("+", _top_right);

	connect(_theme, &QPushButton::clicked, this, &Window::slot_toggle_theme);
	connect(_small, &QPushButton::clicked, this, &Window::slot_text_small);
	connect(_big,   &QPushButton::clicked, this, &Window::slot_text_big);

	_list = new QListWidget(_left);
	_list->setFrameStyle(QFrame::NoFrame);
	connect(_list, &QListWidget::currentItemChanged, this, &Window::slot_item_changed);

	connect(_line, &QLineEdit::textChanged, this, &Window::slot_text_changed);

	_scroll->setFrameStyle(QFrame::NoFrame);
	_scroll->setAlignment(Qt::AlignTop);
	_scroll->setWidgetResizable(true);
	_scroll->setWidget(_view);

	{
		QHBoxLayout * const layout = new QHBoxLayout(_top_left);
		layout->addWidget(_theme, 0, Qt::AlignCenter);
	}

	{
		QVBoxLayout * const layout = new QVBoxLayout(_left);
		layout->setContentsMargins(0,0,0,0);
		layout->setSpacing(0);
		layout->addWidget(_top_left, 0, Qt::AlignTop);
		layout->addWidget(_list, 1);
		_left->setContentsMargins(10,0,10,10);
	}

	{
		QHBoxLayout * const layout = new QHBoxLayout(_top_right);
		layout->addSpacing(10);
		layout->addWidget(_found, 0);
		layout->addStretch(3);
		layout->addWidget(_small, 0);
		layout->addWidget(_big, 0);
		layout->addSpacing(10);
		layout->addWidget(_line, 6);
		_line->setSizePolicy(QSizePolicy::Preferred,
				     QSizePolicy::Preferred);
	}

	{
		QVBoxLayout * const layout = new QVBoxLayout(_right);
		layout->setContentsMargins(0,0,0,0);
		layout->setSpacing(0);
		layout->addWidget(_top_right, 0, Qt::AlignTop);
		layout->addWidget(_scroll, 1);
		_top_right->setContentsMargins(5,5,5,5);
	}

	_split->addWidget(_left);
	_split->addWidget(_right);
	_split->setStretchFactor(0, 1);
	_split->setStretchFactor(1, 1);
	_split->setHandleWidth(1);

	setCentralWidget(_split);

	_line->setFocus();

	update_definition(true);
	update_list_theme();
}

Window::~Window() {}


void Window::closeEvent(QCloseEvent *event) {
	deleteLater();
	QMainWindow::closeEvent(event);
}

void Window::keyPressEvent(QKeyEvent *event) {
	if (	(event->key() == Qt::Key_W ||
		 event->key() == Qt::Key_Q) &&
		(event->modifiers() & Qt::ControlModifier)
	) {
		close();

	} else if (event->key() == Qt::Key_W &&
		   (event->modifiers() & Qt::AltModifier)
	) {
		close();

	} else if (event->key() == Qt::Key_Escape) {
		close();

	} else {
		QMainWindow::keyPressEvent(event);
	}
}

void Window::showEvent(QShowEvent *event) {
	QWidget::showEvent(event);

	QList<int> size = _split->sizes();
	if (size.size() == 2) {
		const int sum = size[0]+size[1];
		size[0] = sum*0.2f;
		size[1] = sum-size[0];
		_split->setSizes(size);
	}

	_top_left->setFixedHeight(_top_right->height());
}

void Window::begin_html(std::ostream &out) const {
	out << "<html lang=\"en\">\n"
		"<head>\n"
		"<meta charset=\"utf-8\">\n";

	out << "<style>\n";

	output_body_css(_dark, out);

	out << "p {\n"
		"  text-align: center;\n"
		"  font-size: 1.5em;\n"
		"  color: #777777;\n"
		"}\n"
		"</style>\n";

	out << "</head>\n";
	out << "<body>\n";
}

void Window::end_html(std::ostream &out) const {
	out << "</body>\n";
}

void Window::definition_of_list_item(std::ostringstream &out) const {

	if (!_list->count()) {
		begin_html(out);
		out << "<p><br>No entries found</p>\n";
		end_html(out);
		return;
	}

	QListWidgetItem * const item = _list->currentItem();
	if (!item) {
		begin_html(out);
		out << "<p><br>No entry selected</p>\n";
		end_html(out);
		return;
	}

	QByteArray ba = item->text().toUtf8();
	const std::string text = ba.data();

	std::ostringstream msg;

	// display definition of selected entry
	if (output_definition(_dict, text, true, _dark, out, msg)) {

		// display error instead of definition
		out.clear();
		out.str("");

		begin_html(out);
		out << "<p><br>" << msg.str() << "</p>\n";
		end_html(out);
	}
}

void Window::update_definition(const bool from_field) {

	std::ostringstream out;

	if (from_field) {

		QSignalBlocker block(_list);

		_list->clear();

		QByteArray ba = _line->text().toUtf8();
		std::string text = ba.data();
		strip(text);

		if (text.empty()) {
			begin_html(out);
			out << "<p><br>Type a word to lookup</p>\n";
			end_html(out);

			_found->setText("0 found");
		} else {
			// fill list with words for which 'text' is a prefix
			list_words(_dict, text,
				   [](const std::string &word, void *data) {
					   QListWidget * const list = (QListWidget*)data;
					   new QListWidgetItem(QString::fromUtf8(word.c_str()), list);
				   }, _list);

			const int count = _list->count();
			_found->setText(QString("%1 found").arg(count));

			if (count > 0) {
				_list->setCurrentItem(_list->item(0));
			}
			definition_of_list_item(out);
		}
	} else {
		definition_of_list_item(out);
	}

	_view->setHtml(QString::fromUtf8(out.str().c_str()));
}

void Window::slot_item_changed(QListWidgetItem *cur, QListWidgetItem *prev) {
	update_definition(false);
}

void Window::slot_text_changed(const QString &) {
	update_definition(true);
}


void Window::set_zoom(double zoom) {
	zoom = std::max(zoom, 0.25);
	zoom = std::min(zoom, 5.0);
	_view->setZoomFactor(zoom);
}

void Window::slot_text_small(bool) {
	set_zoom(_view->zoomFactor() - 0.25);
}

void Window::slot_text_big(bool) {
	set_zoom(_view->zoomFactor() + 0.25);

}

void Window::slot_toggle_theme(bool) {

	_dark = !_dark;

	update_definition(false);
	update_list_theme();
}

void Window::update_list_theme() {
	std::ostringstream css;
	output_color_css(_dark ? "white"   : "black",
			 _dark ? "#252525" : "#dddede", css);
	_list->setStyleSheet(css.str().c_str());
	this->setStyleSheet(css.str().c_str());

	css.str("");
	output_color_css(_dark ? "white"   : "black",
			 _dark ? "#343434" : "#f0f1f1", css);
	_top_right->setStyleSheet(css.str().c_str());

	css.str("");
	output_color_css(_dark ? "white"   : "black",
			 _dark ? "#1d1d1d" : "white", css);
	_line->setStyleSheet(css.str().c_str());
}
