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

class Todo {
 public:
  Todo(int64_t key, string title, bool completed)
      : _key(key), _title(title), _completed(completed) { };

  int64_t GetKey() { return _key; }
  string GetTitle() { return _title; }
  bool GetCompleted() { return _completed; }
  void ToggleCompleted() { _completed = !_completed; }

 private:
  int64_t _key;
  string _title;
  bool _completed;
};

int64_t keyCounter = 1;
map<int64_t, shared_ptr<Todo>> todos;

shared_ptr<Todo> CreateTodo(string title, bool completed) {
  auto key = keyCounter++;
  auto todo = make_shared<Todo>(key, title, completed);
  todos[key] = todo;
  return todo;
}

class TodoAppState : public State, public enable_shared_from_this<TodoAppState>{
 public:
   shared_ptr<Todo> todoEdit = nullptr;

  TodoAppState() {
    CreateTodo("buy milk", false);
    CreateTodo("implement TODO in WASM", true);
    CreateTodo("wash car", false);
  };

  shared_ptr<Node> _renderTodoItem(shared_ptr<Todo> todo) {
    int64_t key = todo->GetKey();
    auto li = El("li");
    li->SetKey(to_string(key));
    auto controls = li->El("div");
    if (todoEdit != nullptr && todoEdit->GetKey() == key) {
      controls->AddClassName("hidden");
    }

    auto checkbox = controls->El("input");
    checkbox->SetAttribute("type", "checkbox");
    if (todo->GetCompleted()) {
      li->AddClassName("completed");
      checkbox->SetAttribute("checked", "");
    }
    checkbox->AddClassName("toggle");
    checkbox->AddEventListener("click", [&, key](const Event& _) {
      todos[key]->ToggleCompleted();
      ScheduleUpdate();
    });

    auto label = controls->El("label");
    label->SetText(todo->GetTitle());

    auto removeButton = controls->El("button");
    removeButton->AddClassName("destroy");
    removeButton->AddEventListener("click", [&, key](const Event& event) {
      todos.erase(key);
      ScheduleUpdate();
    });

    auto editor = li->El("div");
    auto input = editor->El("input");
    input->SetAttribute("type", "text");
    input->AddClassName("edit");
    if (todoEdit != nullptr && todoEdit->GetKey() == key) {
      input->AddClassName("visible");
    }
    input->SetAttribute("value", todo->GetTitle());

    return li;
  }

  string newTitle = "";

  shared_ptr<Node> _renderHeader() {
    auto header = El("header");
    header->SetAttribute("id", "header");
    header->El("h1")->SetText("todos");

    auto input = header->El("input");
    input->SetAttribute("id", "new-todo");
    input->SetAttribute("type", "text");
    input->SetAttribute("placeholder", "What needs to be done?");
    input->SetAttribute("autofocus", "");
    input->SetAttribute("value", newTitle);
    input->AddEventListener("keyup", [&](const Event& event) {
      auto& data = event.GetData();
      if (data.find("keyCode") != data.end() && data.find("value") != data.end()) {
        int keyCode = data["keyCode"].get<int>();
        string value = data["value"].get<string>();
        if (keyCode == 13) {
          CreateTodo(value, false);
          newTitle = "";
        } else {
          newTitle = value;
        }
        ScheduleUpdate();
      }
    });

    return header;
  }

  shared_ptr<Node> _renderTableFooter() {
    auto footer = El("footer");
    footer->SetAttribute("id", "footer");
    footer->El("span")->SetAttribute("id", "todo-count");
    // Dunno what this does, but it's in the angular2 version
    footer->El("div")->AddClassName("hidden");

    auto ul = footer->El("ul");
    ul->SetAttribute("id", "filters");

    auto a1 = ul->El("li")->El("a");
    a1->SetAttribute("href", "#/");
    a1->AddClassName("selected");
    a1->SetText("All");

    auto a2 = ul->El("li")->El("a");
    a2->SetAttribute("href", "#/active");
    a2->SetText("Active");

    auto a3 = ul->El("li")->El("a");
    a3->SetAttribute("href", "#/completed");
    a3->SetText("Completed");

    auto clearCompleted = footer->El("button");
    clearCompleted->SetAttribute("id", "clear-completed");
    clearCompleted->SetText("Clear completed");
    clearCompleted->AddEventListener("click", [&](const Event& _) {
      vector<int64_t> completedKeys;
      for (auto todo : todos) {
        if (todo.second->GetCompleted()) {
          completedKeys.push_back(todo.second->GetKey());
        }
      }
      for (auto k : completedKeys) {
        todos.erase(k);
      }
      ScheduleUpdate();
    });

    return footer;
  }

  shared_ptr<Node> _renderPageFooter() {
    auto footer = El("footer");
    footer->SetAttribute("id", "info");
    footer->El("p")->SetText("Double-click to edit a todo");
    auto p = footer->El("p");
    p->El("span")->SetText("Created using ");
    auto a = p->El("a");
    a->SetAttribute("href", "http://webassembly.org");
    a->SetText("WebAssembly");
    return footer;
  }

  virtual shared_ptr<Node> Build() {
    auto header = _renderHeader();

    auto checkAll = El("input");
    checkAll->SetAttribute("type", "checkbox");
    checkAll->SetAttribute("id", "toggle-all");

    auto checkAllLabel = El("label");
    checkAllLabel->SetAttribute("for", "toggle-all");
    checkAllLabel->SetText("Mark all as complete");

    auto todoUl = El("ul");
    todoUl->SetAttribute("id", "todo-list");
    for (auto i = todos.begin(); i != todos.end(); i++) {
      auto todo = i->second;
      todoUl->AddChild(_renderTodoItem(todo));
    }

    auto mainSection = El("section");
    mainSection->SetAttribute("id", "main");
    mainSection->Nest({
        checkAll,
        checkAllLabel,
        todoUl,
    });

    auto appSection = El("section");
    appSection->SetAttribute("id", "todoapp");
    appSection->Nest({
        header,
        mainSection,
        _renderTableFooter(),
    });

    return El("div")->Nest({
        appSection,
        _renderPageFooter(),
    });
  }
};

class TodoApp : public StatefulWidget {
 public:
  TodoApp() : StatefulWidget() {}

  virtual shared_ptr<State> CreateState() {
    return make_shared<TodoAppState>();
  }
};

#endif //TODO_WIDGETS_H
