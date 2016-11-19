## Параллельное программирование на C++

Неблокирующая синхронизация



### План

* Определения. Иерархия Blocking/Lock-free/Wait-free
* Compare-And-Swap. std::atomic::compare_exchange_*
* Lock-free stack
* Lock-free queue



## Определения. Иерархия Blocking/Lock-free/Wait-free

Блокирующие структуры данных - такие структуры данных, которые для безопасной работы в многопоточной среде используют мьютексы, условные переменные, future и т.п.

Неблокирующие структуры данных - структуры данных, безопасно работающие в многопоточной среде, не использующие механизмы блокировок.


Структуры данных, свободные от блокировок - структуры данных, с которымы можно безопасно работать в многопоточной среде, не прибегая к блокировкам.

**Не все неблокирующие СД являются свободными от блокировок!**

**Пример:** spinlock_mutex


```
class Spinlock
{
public:
    Spinlock()
        : locked_(false)
    {}

    void lock()
    {
        while (locked_.exchange(true)) {
            // wait
        }
    }

    void unlock() {
        locked_.store(false);
    }

private:
    std::atomic<bool> locked_;
};
```

Здесь не вызываются никакие блокирующие функции. Но тем не менее, это все тот же мьютекс, приводящий к блокировкам.


**Пример: мьютекс Питерсона**
```
flag[i] = true
victim = i
while flag[1-i] and victim == i:
  pass
```

Если поток снимется с исполнения в момент, между flag[i] = true и victim = i, то для другого потока не будет никакого прогресса в методе lock().


Метод свободен от блокировок (lock-free), если хотя бы один из вызовов метода продвигается вперед, т.е. имеет место глобальный прогресс, вне зависимости от поведения других потоков.

При этом каждый конкретный вызов может бесконечно голодать, но при условии, что завершаются конкурентные вызовы.

**Более строго:** При бесконечном исполнении вызовы метода завершаются бесконечно много раз.


Свободная от блокировок СД называется свободной от ожидания (wait-free), если обладает дополнительным свойством: каждый обращающийся к ней поток может завершить свою работу за ограниченное количество шагов вне зависимости от поведения других потоков.

*Свобода от ожидания - скорее теоретический интерес, на практике достаточно свободы от блокировок.*


#### Плюсы lock-free и wait-free алгоритмов
* Высокий уровень параллелизма - какой-либо поток продвигается на каждом шаге
* Надежность - если поток завершился, не сняв блокировку, СД испорчена
* Невозможность появления deadlock'ов (но возможно возникновение livelock'ов - каждый поток начинает все сначала)

#### Минусы lock-free и wait-free алгоритмов
* Сложность реализации - для реализации простых СД требуется много затрат
* Производительность может просесть - атомарные операции более дорогостоящие, и используются в большом количестве



## Compare-And-Swap. std::atomic::compare\_exchange\_*

```
atomic {
  current = var
  if (current == expected) {
    var = desired
    return true
  } else {
    expected = current
    return false
  }
}
```


CAS – самая выразительная RMW-операция!

С помощью CAS легко атомарно выполнить любое преобразование вида x → f (x)


В С++ две версии CAS – слабая и сильная:
* atomic<T>::compare_exchange_weak(...)
* atomic<T>::compare_exchange_strong(...)

Слабая более производительна на некоторых платформах, но возможно ложное поведение.

\* [Про memory_order](https://habrahabr.ru/post/197520/)


Мы отказываемся от сериализации доступа с помощью мьютексов и спинлоков и позволяем потокам конкурентно работать с внутренностями структуры данных.

Гораздо больше промежуточных публичных состояний структуры данных.



## Lock-free stack


Будем хранить элементы как односвязный список. Тогда добавление элемента:
1. Создать новый узел
2. Записать в его указатель next текущее значение head
3. Записать в head указатель на новый узел

**Важно, чтобы новый узел был подготовлен до того, как на него начнет указывать head**


**Операция push**
```
void push(const T& data) {
  Node* const newNode = new Node(data);
  newNode->next = head.load();
  while (!head.compare_exchange_weak(newNode->next, newNode));
}
```


Извлечение элемента:
1. Прочитать текущее значение head
2. Прочитать head->next
3. Записать в head значение head->next
4. Вернуть data
5. Удалить узел

*Шаг 5 - самый сложный, так как если два потока одновременно прочитают head, а затем один дойдет до шага 5, то другой получит висячий указатель который будет пытаться разыменовывать.*


**Операция pop (без заботы об удалении)**
```
std::shared_ptr<T> pop() {
  Node* oldHead = head.load();
  while (oldHead &&
    !head.compare_exchange_weak(oldHead, oldHead->next));
  return oldHead ? oldHead->data : nullptr;
}
```


**Операция pop (со "сборщиком мусора")**
```
std::shared_ptr<T> pop() {
  ++threadsInPop;
  Node* oldHead = head.load();
  while (oldHead &&
    !head.compare_exchange_weak(oldHead, oldHead->next));
  auto res = oldHead ? oldHead->data : nullptr;
  tryReclaim(oldHead);
  return res;
}
```


**Операция tryReclaim**
```
void tryReclaim(Node* oldHead)
{
    if (threadsInPop_ == 1) {
        Node* nodesToDelete = toBeDeleted.exchange(nullptr);
        if (!--threadsInPop) {
            deleteNodes(nodesToDelete);
        } else if (nodesToDelete) {
            chainPendingNodes(nodesToDelete);
        }
        delete oldHead;
    } else {
        chainPendingNode(oldHead);
        --threadsInPop;
    }
}
```


#### Другое решение - указатели опасности

При работе с узлом устанавливается специальный "указатель опасности", который сигнализирует о том, что данный узел еще кем-то используется и его нельзя удалять.

*Указатели опасности защищены патентной заявкой, поданной IBM :(*


**Операция pop (с указателями опасности)**
```
std::shared_ptr<T> pop()
{
    std::atomic<void*>& hp = getHazardPointerForCurrentThread();
    Node* oldHead = head_.load();
    do {
        Node* tmp = nullptr;
        do {
            tmp = oldHead;
            hp.store(oldHead);
            oldHead = head_.load();
        } while (oldHead != tmp);
    } while (oldHead && !head_.compare_exchange_strong(oldHead, oldHead->next));
    hp.store(nullptr);
    std::shared_ptr<T> res;
    if (oldHead) {
        res.swap(oldHead->data);
        if (outstandingHazardPointersFor(oldHead)) {
            reclaimLater(oldHead);
        } else {
            delete oldHead;
        }
        deleteNodesWithNoHazards();
    }
    return res;
}
```



## Lock-free queue


Работаем точно так же, как и со стеком, за исключением того, что есть head и tail.

**Операция pop**
```
Node* popHead() {
  Node* const oldHead = head.load();
  if (oldHead == tail.load()) {
    return nullptr;
  }
  head.store(oldHead->next);
  return oldHead;
}

std::shared_ptr<T> pop() {
  Node* oldHead = popHead();
  if (!oldHead) {
    return nullptr;
  }
  std::shared_ptr<T> res = oldHead->data;
  delete oldHead;
  return res;
}
```


**Операция push**
```
void push(const T& newValue) {
  std::shared_ptr<T> newData(std::make_shared<T>(newValue));
  Node* p = new Node;
  Node* const oldTail = tail.load();
  oldTail->data.swap(newData);
  oldTail->next = p;
  tail.store(p);
}
```


Все работает замечательно когда у нас один поток вызывает push(), а другой pop() (т.е. один продьюсер и один консьюмер)

Какие возникают проблемы, если несколько потоков вызывают push() и pop()?


Для метода pop() решаем аналогично тому, как решали в стеке. Надо решить для push().

Один вариант решения - добавлять фиктивные узлы между реальными.


Другой вариант - сделать данные атомарными (std::atomic<T*>), и заменять из compare_exchange_strong.
