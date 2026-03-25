# Contributing Guidelines

Спасибо за интерес к проекту! Это образовательный проект для демонстрации работы с UEFI.

## Как внести вклад

1. **Fork** репозиторий
2. Создайте **feature branch**: `git checkout -b feature/my-improvement`
3. **Commit** изменения: `git commit -am 'Add some feature'`
4. **Push** в branch: `git push origin feature/my-improvement`
5. Создайте **Pull Request**

## Стиль кода

### C Code Style

- **Отступы**: 4 пробела (не табы)
- **Фигурные скобки**: K&R style
  ```c
  if (condition) {
      do_something();
  }
  ```
- **Именование**:
  - `snake_case` для функций и переменных
  - `UPPER_CASE` для макросов и констант
  - `PascalCase` для типов (соответствует UEFI spec)
- **Максимальная длина строки**: 100 символов
- **Комментарии**: Doxygen-style для публичных API

### Пример

```c
/**
 * Calculate SHA-256 hash of data buffer.
 *
 * @param data  Input data to hash
 * @param len   Length of input data in bytes
 * @param hash  Output buffer (32 bytes) for hash result
 */
void sha256_hash(const uint8_t* data, size_t len, uint8_t* hash) {
    sha256_context ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}
```

## Требования к PR

- [ ] Код компилируется без предупреждений (`/W4`)
- [ ] Новый функционал покрыт комментариями
- [ ] Обновлен README.md при необходимости
- [ ] Соблюдается лицензия MIT

## Безопасность

Если вы обнаружили уязвимость безопасности:
1. **Не создавайте public issue**
2. Отправьте описание на email автора
3. Дождитесь исправления перед раскрытием

## Вопросы?

Создавайте GitHub Issue для:
- Вопросов об использовании
- Предложений по улучшению
- Сообщений об ошибках

## Code of Conduct

- Будьте уважительны
- Конструктивная критика приветствуется
- Помогайте новичкам
