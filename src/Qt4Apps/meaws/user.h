#ifndef USER_H
#define USER_H

#include "defs.h"

#include <QDialog>
#include <QInputDialog>
#include <QErrorMessage>
#include <QStringList>
#include <QtGui>
//#include <QList>

class QAction;
class QMenu;
class QTextEdit;
class QLabel;
class QErrorMessage;

class User : public QDialog
{
	Q_OBJECT

public:
	User();
	~User();
	QString getName();

public slots:
	void newUser();
	void open();
	bool save();
	bool saveAs();
	bool close();
	void setUserInfo();

signals:
	void enableActions(int state);

private slots:
	bool queryName();
	bool queryLevel();
	bool queryWeekPractice();
	bool queryWeekPlay();
	bool queryYearsPlaying();

private:
	bool maybeSave();
	bool saveFile(const QString &saveFilename);
	void openFile(const QString &openFilename);
	void updateUserInfoDisplay();

	QStringList levelList_;
	QStringList weekPracticeList_;
	QStringList weekPlayList_;
	QStringList yearsPlayingList_;

	bool isModified_;
	QString username_;
	QString filename_;
	QString level_;
	QString weekPractice_;
	QString weekPlay_;
	QString yearsPlaying_;

	QPushButton *usernameButton;
	QPushButton *levelButton;
	QPushButton *weekPracticeButton;
	QPushButton *weekPlayButton;
	QPushButton *yearsPlayingButton;
	QPushButton *okButton;
	
};
#endif

