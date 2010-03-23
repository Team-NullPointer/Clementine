/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "analyzercontainer.h"
#include "baranalyzer.h"
#include "blockanalyzer.h"
#include "boomanalyzer.h"
#include "sonogram.h"
#include "turbine.h"

#include <QMouseEvent>
#include <QHBoxLayout>
#include <QSettings>
#include <QtDebug>

const char* AnalyzerContainer::kSettingsGroup = "Analyzer";

AnalyzerContainer::AnalyzerContainer(QWidget *parent)
  : QWidget(parent),
    context_menu_(new QMenu(this)),
    group_(new QActionGroup(this)),
    mapper_(new QSignalMapper(this)),
    current_analyzer_(NULL),
    engine_(NULL)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  setLayout(layout);
  layout->setContentsMargins(0, 0, 0, 0);

  AddAnalyzerType<BarAnalyzer>();
  AddAnalyzerType<BlockAnalyzer>();
  AddAnalyzerType<BoomAnalyzer>();
  AddAnalyzerType<Sonogram>();
  AddAnalyzerType<TurbineAnalyzer>();
  connect(mapper_, SIGNAL(mapped(int)), SLOT(ChangeAnalyzer(int)));

  context_menu_->addSeparator();

  disable_action_ =
      context_menu_->addAction(tr("No analyzer"), this, SLOT(DisableAnalyzer()));
  disable_action_->setCheckable(true);
  group_->addAction(disable_action_);

  Load();
}

void AnalyzerContainer::mouseReleaseEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton || e->button() == Qt::RightButton)
    context_menu_->popup(e->globalPos());
}

void AnalyzerContainer::set_engine(EngineBase *engine) {
  if (current_analyzer_)
    current_analyzer_->set_engine(engine);
  engine_ = engine;
}

void AnalyzerContainer::DisableAnalyzer() {
  delete current_analyzer_;
  current_analyzer_ = NULL;

  Save();
}

void AnalyzerContainer::ChangeAnalyzer(int id) {
  QObject* instance = analyzer_types_[id]->newInstance(Q_ARG(QWidget*, this));

  if (!instance) {
    qWarning() << "Couldn't intialise a new" << analyzer_types_[id]->className();
    return;
  }

  delete current_analyzer_;
  current_analyzer_ = qobject_cast<Analyzer::Base*>(instance);
  current_analyzer_->set_engine(engine_);

  layout()->addWidget(current_analyzer_);

  Save();
}

void AnalyzerContainer::Load() {
  QSettings s;
  s.beginGroup(kSettingsGroup);

  QString type = s.value("type", "BlockAnalyzer").toString();
  if (type.isEmpty()) {
    DisableAnalyzer();
    disable_action_->setChecked(true);
  } else {
    for (int i=0 ; i<analyzer_types_.count() ; ++i) {
      if (type == analyzer_types_[i]->className()) {
        ChangeAnalyzer(i);
        actions_[i]->setChecked(true);
        break;
      }
    }
  }
}

void AnalyzerContainer::Save() {
  QSettings s;
  s.beginGroup(kSettingsGroup);

  s.setValue("type", current_analyzer_ ?
             current_analyzer_->metaObject()->className() :
             QVariant());
}
