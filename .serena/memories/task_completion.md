# Task Completion Checklist for MetaHookSv

## When a Task is Completed

### 1. Code Quality Checks
- [ ] Code follows project naming conventions (g_ prefix for globals, p for pointers, etc.)
- [ ] Code uses appropriate Hungarian notation
- [ ] Engine compatibility checks are in place where needed
- [ ] No hardcoded values that should be configurable
- [ ] Memory management is proper (no leaks)

### 2. Build Verification
- [ ] Code compiles without errors in Release configuration
- [ ] Code compiles without errors in Debug configuration
- [ ] If applicable, code compiles in Release_AVX2 configuration
- [ ] No new compiler warnings introduced

### 3. Testing
- [ ] Manual testing performed with target game
- [ ] Tested on appropriate engine versions
- [ ] No crashes or unexpected behavior
- [ ] Performance impact is acceptable

### 4. Documentation
- [ ] Code comments added for complex logic
- [ ] If new feature, update relevant documentation in docs/
- [ ] If new CVars added, document them
- [ ] If new entity types added, document them

### 5. Integration
- [ ] Plugin load order considerations checked
- [ ] Dependencies on other plugins verified
- [ ] No conflicts with existing functionality

### 6. Changelog
- [ ] Update changelog following the format:
  ```
  **changes**
  [PluginName] Description of change
  
  **改动**
  [PluginName] 改动描述
  ```

## DO NOT Run Automatically
- **DO NOT** run tests automatically unless explicitly requested
- **DO NOT** run build commands automatically unless explicitly requested
- **DO NOT** commit changes automatically unless explicitly requested

## Ask User Before
- Running any build or test commands
- Making significant architectural changes
- Adding new dependencies
- Modifying plugin load order
