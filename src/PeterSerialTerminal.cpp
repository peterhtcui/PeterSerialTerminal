/*
 * MIT License
 *
 * Copyright (c) 2018-2021 Peter
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*!
 * \file PeterSerialTerminal.cpp
 * \brief Serial terminal library for Arduino.
 * \details
 *      Source:         https://github.com/Peter/PeterSerialTerminal
 *      Documentation:  https://peterhtcui.github.io/PeterSerialTerminal
 */

#include "PeterSerialTerminal.h"

/*!
 * \brief SerialTerminal constructor.
 * \param newlineChar
 *      Newline character \c '\\r' or \c '\\n'. \n
 *      Default: \c '\\n'.
 * \param delimiterChar
 *      Delimiter separator character between commands and arguments.\n
 *      Default: space.
 */
SerialTerminal::SerialTerminal(char newlineChar, char delimiterChar) :
        _commandList(NULL),
        _numCommands(0),
        _newlineChar(newlineChar),
        _lastPos(NULL),
        _defaultHandler(NULL),
        _historyCount(0),
        _historyIndex(0),
        _currentHistoryIndex(-1),
        _inHistoryMode(false),
        _tabIndex(0),
        _tabMode(false)
{
    // strtok_r needs a null-terminated string
    _delimiter[0] = delimiterChar;
    _delimiter[1] = '\0';

    // Clear receive buffer
    clearBuffer();
    
    // Initialize history
    for (int i = 0; i < ST_MAX_HISTORY_ENTRIES; i++) {
        _history[i][0] = '\0';
    }
}

/*!
 * \brief Add command with callback handler.
 * \param command
 *      Register a null-terminated ASCII command.
 * \param function
 *      The function to be called when receiving the \c command.
 */
void SerialTerminal::addCommand(const char *command, void (*function)())
{
    // Increase size command list by one
    _commandList = (SerialTerminalCallback *)realloc(_commandList,
            sizeof(SerialTerminalCallback) * (_numCommands + 1));

    // Copy command and store command callback handler
    strncpy(_commandList[_numCommands].command, command, ST_NUM_COMMAND_CHARS);
    _commandList[_numCommands].function = function;

    // Increment number of commands
    _numCommands++;
}


/*!
 * \brief Set the control state to echo any printable chars to the console.
 * \details
 *      Set the control state to echo any printable chars to the console.
 * \param doEcho
 *      Should all printable chars be echoed to the serial console?
 */
void SerialTerminal::setSerialEcho(bool doEcho)
{
    doCharEcho = doEcho;
}

/*!
 * \brief Set post command handler callback for after all handled commands.
 * \details
 *      Store post command handler which will be called when after executing any handled command.
 * \param function
 *      Address of the callback function. This function will be called after any handled event.
 */
void SerialTerminal::setPostCommandHandler(void (*function)(void))
{
    _postCommandHandler = function;
}

/*!
 * \brief Set default callback handler for unknown commands.
 * \details
 *      Store default callback handler which will be called when receiving an unknown command.
 * \param function
 *      Address of the default handler. This function will be called when the command is not
 *      recognized. The parameter contains the first ASCII command.
 */
void SerialTerminal::setDefaultHandler(void (*function)(const char *))
{
    _defaultHandler = function;
}

/*!
 * \brief Read from serial port.
 * \details
 *      Process command when newline character has been received.
 */
void SerialTerminal::readSerial()
{
    bool matched = false;
    char *command;
    char c;

    while (Serial.available() > 0) {
        // Read one character from serial port
        c = (char)Serial.read();

        // Handle special keys first
        if (c == 0x1B) { // ESC sequence start
            // Wait for the next character to determine the key
            if (Serial.available() > 0) {
                char c2 = Serial.read();
                if (c2 == '[') {
                    // Wait for the third character
                    if (Serial.available() > 0) {
                        char c3 = Serial.read();
                        handleSpecialKeys(c3);
                    }
                }
            }
            continue;
        }

        // Check newline character \c '\\r' or \c '\\n' at the end of a command
        if (c == _newlineChar) {
            // Echo received char
            if (doCharEcho) {
                Serial.println();
            }

            // Add to history if not empty and not in tab mode
            if (_rxBufferIndex > 0 && !_tabMode) {
                addToHistory(_rxBuffer);
            }

            // Reset history mode
            _inHistoryMode = false;
            _currentHistoryIndex = -1;
            resetTabMode();

            // Search for command at start of buffer
            command = strtok_r(_rxBuffer, _delimiter, &_lastPos);

            if (command != NULL) {
                for (int i = 0; i < _numCommands; i++) {
                    // Compare the found command against the list of known commands for a match
                    if (strncmp(command, _commandList[i].command, ST_NUM_COMMAND_CHARS) == 0) {
                        // Call command callback handler
                        (*_commandList[i].function)();
                        matched = true;
                        break;
                    }
                }

                if (!matched && (_defaultHandler != NULL)) {
                    (*_defaultHandler)(command);
                }
            }

            // Run the post command handler
            if (_postCommandHandler) {
                (*_postCommandHandler)();
            }

            clearBuffer();
        }
        // Handle backspace and delete
        else if (c == '\b' || c == 127) {
            if (_rxBufferIndex > 0) {
                _rxBufferIndex--;
                _rxBuffer[_rxBufferIndex] = '\0';
                if (doCharEcho) {
                    Serial.print("\b \b"); // 1 char back, space, 1 char back
                }
            }
            resetTabMode();
        }
        // Handle Tab key for command completion
        else if (c == '\t') {
            if (_rxBufferIndex > 0) {
                completeCommand();
            }
        }
        // Handle printable characters
        else if (isprint(c)) {
            // Store printable characters in serial receive buffer
            if (_rxBufferIndex < ST_RX_BUFFER_SIZE) {
                _rxBuffer[_rxBufferIndex++] = c;
                _rxBuffer[_rxBufferIndex] = '\0';
                // Echo received char
                if (doCharEcho) {
                    Serial.print(c);
                }
            }
            resetTabMode();
        }
    }
}

/*!
 * \brief Clear serial receive buffer.
 */
void SerialTerminal::clearBuffer()
{
    _rxBuffer[0] = '\0';
    _rxBufferIndex = 0;
}

/*!
 * \brief Get next argument.
 * \return
 *      Address: Pointer to next argument\n
 *      NULL: No argument available
 */
char *SerialTerminal::getNext()
{
    return strtok_r(NULL, _delimiter, &_lastPos);
}

/*!
 * \brief Get all remaining characters from serial buffer.
 * \return
 *      Address: Pointer to remaining characters in serial receive buffer.\n
 *      NULL: No remaining data available.
 */
char *SerialTerminal::getRemaining()
{
    return strtok_r(NULL, "", &_lastPos);
}

/*!
 * \brief Handle special keys (arrow keys, etc.)
 */
void SerialTerminal::handleSpecialKeys(char c)
{
    switch (c) {
        case 'A': // Up arrow
            navigateHistory(-1);
            break;
        case 'B': // Down arrow
            navigateHistory(1);
            break;
        case 'C': // Right arrow
            // Could be used for cursor movement in the future
            break;
        case 'D': // Left arrow
            // Could be used for cursor movement in the future
            break;
    }
}

/*!
 * \brief Navigate through command history
 */
void SerialTerminal::navigateHistory(int direction)
{
    if (_historyCount == 0) return;

    if (!_inHistoryMode) {
        _inHistoryMode = true;
        _currentHistoryIndex = _historyCount - 1;
    } else {
        _currentHistoryIndex += direction;
        if (_currentHistoryIndex < 0) {
            _currentHistoryIndex = 0;
        } else if (_currentHistoryIndex >= _historyCount) {
            _currentHistoryIndex = _historyCount - 1;
        }
    }

    // Clear current line and show history entry
    if (doCharEcho) {
        clearLine();
        if (_currentHistoryIndex >= 0 && _currentHistoryIndex < _historyCount) {
            strcpy(_rxBuffer, _history[_currentHistoryIndex]);
            _rxBufferIndex = strlen(_rxBuffer);
            Serial.print(_rxBuffer);
        }
    }
}

/*!
 * \brief Complete command using Tab key
 */
void SerialTerminal::completeCommand()
{
    char matches[ST_MAX_HISTORY_ENTRIES][ST_NUM_COMMAND_CHARS + 1];
    int matchCount = 0;

    // Find matching commands
    findMatchingCommands(_rxBuffer, matches, &matchCount);

    if (matchCount == 0) {
        // No matches found
        if (doCharEcho) {
            Serial.print('\a'); // Bell sound
        }
    } else if (matchCount == 1) {
        // Single match - complete the command
        if (doCharEcho) {
            clearLine();
            strcpy(_rxBuffer, matches[0]);
            _rxBufferIndex = strlen(_rxBuffer);
            Serial.print(_rxBuffer);
        }
    } else {
        // Multiple matches - show all possibilities
        if (doCharEcho) {
            Serial.println();
            for (int i = 0; i < matchCount; i++) {
                Serial.print(matches[i]);
                Serial.print(" ");
            }
            Serial.println();
            printPrompt();
            Serial.print(_rxBuffer);
        }
    }
}

/*!
 * \brief Reset tab completion mode
 */
void SerialTerminal::resetTabMode()
{
    _tabMode = false;
    _tabIndex = 0;
}

/*!
 * \brief Print command prompt
 */
void SerialTerminal::printPrompt()
{
    Serial.print("> ");
}

/*!
 * \brief Clear current line
 */
void SerialTerminal::clearLine()
{
    // Move cursor to beginning of line
    Serial.print("\r");
    // Clear the line by printing spaces (more compatible than ANSI escape sequences)
    for (int i = 0; i < 80; i++) {
        Serial.print(" ");
    }
    // Move cursor back to beginning of line
    Serial.print("\r");
    // Print prompt
    printPrompt();
}

/*!
 * \brief Move cursor left
 */
void SerialTerminal::moveCursorLeft(int positions)
{
    for (int i = 0; i < positions; i++) {
        Serial.print("\b");
    }
}

/*!
 * \brief Move cursor right
 */
void SerialTerminal::moveCursorRight(int positions)
{
    for (int i = 0; i < positions; i++) {
        Serial.print(" ");
    }
}

/*!
 * \brief Check if a command matches a partial string
 */
bool SerialTerminal::isCommandMatch(const char *partial, const char *command)
{
    return strncmp(partial, command, strlen(partial)) == 0;
}

/*!
 * \brief Find all commands that match the partial string
 */
void SerialTerminal::findMatchingCommands(const char *partial, char matches[][ST_NUM_COMMAND_CHARS + 1], int *matchCount)
{
    *matchCount = 0;
    
    for (int i = 0; i < _numCommands && *matchCount < ST_MAX_HISTORY_ENTRIES; i++) {
        if (isCommandMatch(partial, _commandList[i].command)) {
            strcpy(matches[*matchCount], _commandList[i].command);
            (*matchCount)++;
        }
    }
}

/*!
 * \brief Add command to history
 */
void SerialTerminal::addToHistory(const char *command)
{
    // Don't add empty commands
    if (strlen(command) == 0) return;
    
    // Don't add duplicate of the last command
    if (_historyCount > 0 && strcmp(_history[_historyCount - 1], command) == 0) {
        return;
    }

    // Shift history if we've reached the maximum
    if (_historyCount >= ST_MAX_HISTORY_ENTRIES) {
        for (int i = 0; i < ST_MAX_HISTORY_ENTRIES - 1; i++) {
            strcpy(_history[i], _history[i + 1]);
        }
        _historyCount = ST_MAX_HISTORY_ENTRIES - 1;
    }

    // Add new command to history
    strncpy(_history[_historyCount], command, ST_HISTORY_ENTRY_SIZE - 1);
    _history[_historyCount][ST_HISTORY_ENTRY_SIZE - 1] = '\0';
    _historyCount++;
}

/*!
 * \brief Show command history
 */
void SerialTerminal::showHistory()
{
    if (_historyCount == 0) {
        Serial.println("No command history available.");
        return;
    }

    Serial.println("Command History:");
    for (int i = 0; i < _historyCount; i++) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(_history[i]);
    }
}

/*!
 * \brief Clear command history
 */
void SerialTerminal::clearHistory()
{
    for (int i = 0; i < ST_MAX_HISTORY_ENTRIES; i++) {
        _history[i][0] = '\0';
    }
    _historyCount = 0;
    _currentHistoryIndex = -1;
    _inHistoryMode = false;
}
