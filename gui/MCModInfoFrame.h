#ifndef MCMODINFOFRAME_H
#define MCMODINFOFRAME_H

#include <QFrame>
#include "logic/Mod.h"

namespace Ui {
class MCModInfoFrame;
}

class MCModInfoFrame : public QFrame
{
	Q_OBJECT
	
public:
	explicit MCModInfoFrame(QWidget *parent = 0);
	~MCModInfoFrame();

	void setName(QString name);
	void setDescription(QString description);
	void setAuthors(QString authors);
	void setCredits(QString credits);
	void setWebsite(QString website);

	
private:
	Ui::MCModInfoFrame *ui;
};

void handleModInfoUpdate(Mod &m, MCModInfoFrame *frame);

#endif // MCMODINFOFRAME_H
