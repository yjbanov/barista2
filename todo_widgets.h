#ifndef TODO_WIDGETS_H
#define TODO_WIDGETS_H

#include <iostream>
#include <list>
#include <locale>
#include <codecvt>
#include <string>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

vector<string> statuses = {
    "Planned",
    "Pitched",
    "Won",
    "Lost",
};

vector<string> randomStrings = {
    "Foo",
    "Bar",
    "Baz",
    "Qux",
    "Quux",
    "Garply",
    "Waldo",
    "Fred",
    "Plugh",
    "Waldo",
    "Xyzzy",
    "Thud",
    "Cruft",
};

class Row {
 public:
  vector<string> columns;
  string status;
};

class SampleAppState : public State, public enable_shared_from_this<SampleAppState>{
 public:
  int keyCounter = 1;
  bool greet = true;
  map<int, Row> rows;

  SampleAppState() {
    for (int i = 0; i < 10; i++) {
      AddRow();
    }
  };

  void AddRow() {
    Row row;
    row.status = statuses[rand() % statuses.size()];
    int len = (int) randomStrings.size();
    for (int i = 0; i < 10; i++) {
      row.columns.push_back(randomStrings[rand() % len]);
    }
    rows[keyCounter++] = row;
  }

  virtual shared_ptr<Node> Build() {
    auto container = El("div");

    auto text = greet ? Tx("Hello") : Tx("Ciao!!!");

    auto table = El("div");
    table->SetKey("table");
    table->AddClassName("table");
    auto thiz = shared_from_this();
    for (auto r = rows.begin(); r != rows.end(); r++) {
      auto row = table->El("div");
      int key = r->first;
      row->SetKey(to_string(key));
      row->AddClassName("row");

      auto keyCell = row->El("div");
      keyCell->AddClassName("cell");
      keyCell->SetText(to_string(key));

      for (string cellData : r->second.columns) {
        auto cell = row->El("div");
        cell->AddClassName("cell");
        cell->SetText(cellData);
      }

      auto statusCell = row->El("div");
      statusCell->AddClassName("status-cell");
      for (string status : statuses) {
        auto statusButton = statusCell->El("button");
        if (status == r->second.status) {
          statusButton->AddClassName("active-status");
        }
        statusButton->SetText(status);
        statusButton->AddEventListener("click", [key, thiz, status]() {
          cout << "Changing status from " << thiz->rows[key].status << " to " << status << endl;
          thiz->rows[key].status = status;
          thiz->ScheduleUpdate();
        });
      }

      auto removeButton = row->El("div")->El("button");
      removeButton->SetText("Remove");
      // TODO(yjbanov): this probably creates a cycle between <button> and SampleAppState
      removeButton->AddEventListener("click", [key, thiz]() {
        thiz->rows.erase(key);
        thiz->ScheduleUpdate();
      });
    }

    auto button = El("button");
    button->SetText("Add Row");
    button->AddEventListener("click", [&]() {
      cout << "Clicked! " << greet << endl;
      greet = !greet;
      AddRow();
      ScheduleUpdate();
    });

    container->AddChild(button);
    container->AddChild(text);
    container->AddChild(table);
    return container;
  }
};

class SampleApp : public StatefulWidget {
 public:
  SampleApp() : StatefulWidget() {}

  virtual shared_ptr<State> CreateState() {
    return make_shared<SampleAppState>();
  }
};

#endif //TODO_WIDGETS_H
