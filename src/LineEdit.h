#ifndef INCLUDED_LINEEDIT_H
#define INCLUDED_LINEEDIT_H

#include <QtWidgets/QLineEdit>

class LineEdit : public QLineEdit {
Q_OBJECT
public:
	LineEdit(const std::string &text, QWidget *parent);
	virtual ~LineEdit();

protected:
	virtual void keyPressEvent(QKeyEvent *event);
};

#endif
