/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "LookupServerAddress.h"

#include <launch/LaunchTask.h>

LookupServerAddress::LookupServerAddress(LaunchTask *parent) :
    LaunchStep(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup(new QDnsLookup(this))
{
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup, &QDnsLookup::finished, this, &LookupServerAddress::on_dnsLookupFinished);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->setType(QDnsLookup::SRV);
}

void LookupServerAddress::setLookupAddress(const QString &lookupAddress)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lookupAddress = lookupAddress;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->setName(QString("_minecraft._tcp.%1").arg(lookupAddress));
}

void LookupServerAddress::setOutputAddressPtr(MinecraftServerTargetPtr output)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output = std::move(output);
}

bool LookupServerAddress::abort()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->abort();
    emitFailed("Aborted");
    return true;
}

void LookupServerAddress::executeTask()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->lookup();
}

void LookupServerAddress::on_dnsLookupFinished()
{
    if (isFinished())
    {
        // Aborted
        return;
    }

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->error() != QDnsLookup::NoError)
    {
        emit logLine(QString("Failed to resolve server address (this is NOT an error!) %1: %2\n")
            .arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->name(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->errorString()), MessageLevel::Launcher);
        resolve(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lookupAddress, 25565); // Technically the task failed, however, we don't abort the launch
                                                      // and leave it up to minecraft to fail (or maybe not) when connecting
        return;
    }

    const auto records = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->serviceRecords();
    if (records.empty())
    {
        emit logLine(
                QString("Failed to resolve server address %1: the DNS lookup succeeded, but no records were returned.\n")
                .arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->name()), MessageLevel::Warning);
        resolve(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lookupAddress, 25565); // Technically the task failed, however, we don't abort the launch
                                                      // and leave it up to minecraft to fail (or maybe not) when connecting
        return;
    }

    const auto &firstRecord = records.at(0);
    quint16 port = firstRecord.port();

    emit logLine(QString("Resolved server address %1 to %2 with port %3\n").arg(
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->name(), firstRecord.target(), QString::number(port)),MessageLevel::Launcher);
    resolve(firstRecord.target(), port);
}

void LookupServerAddress::resolve(const QString &address, quint16 port)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output->address = address;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output->port = port;

    emitSucceeded();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dnsLookup->deleteLater();
}
