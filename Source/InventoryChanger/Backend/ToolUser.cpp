#include "BackendSimulator.h"
#include "ToolUser.h"

#include <InventoryChanger/ItemGenerator.h>

namespace inventory_changer::backend
{

std::optional<Response> ToolUser::applySticker(std::list<inventory::Item_v2>::iterator item, std::list<inventory::Item_v2>::const_iterator sticker, std::uint8_t slot)
{
    const auto skin = item->get<inventory::Skin>();
    if (!skin)
        return {};

    skin->stickers[slot].stickerID = StaticData::lookup().getStorage().getStickerKit(sticker->gameItem()).id;
    skin->stickers[slot].wear = 0.0f;

    backend.moveToFront(item);
    backend.removeItem(sticker);
    return Response{ Response::StickerApplied{ item, slot } };
}

void ToolUser::activateOperationPass(std::list<inventory::Item_v2>::const_iterator item)
{
    const auto& gameItem = item->gameItem();
    if (!gameItem.isOperationPass())
        return;

    const auto coinID = gameItem.getWeaponID() != WeaponId::OperationHydraPass ? static_cast<WeaponId>(static_cast<int>(gameItem.getWeaponID()) + 1) : WeaponId::BronzeOperationHydraCoin;
    if (const auto operationCoin = StaticData::lookup().findItem(coinID); operationCoin.has_value()) {
        backend.addItem(inventory::Item_v2{ *operationCoin });
        backend.removeItem(item);
    }
}

std::optional<Response> ToolUser::activateViewerPass(std::list<inventory::Item_v2>::const_iterator item)
{
    const auto& gameItem = item->gameItem();
    if (!gameItem.isViewerPass())
        return {};

    const auto coinID = static_cast<WeaponId>(static_cast<int>(gameItem.getWeaponID()) + 1);
    if (const auto eventCoin = StaticData::lookup().findItem(coinID); eventCoin.has_value()) {
        const auto addedEventCoin = backend.addItem(inventory::Item_v2{ *eventCoin });
        backend.removeItem(item);
        return Response{ Response::ViewerPassActivated{ addedEventCoin } };
    }
    return {};
}

std::optional<Response> ToolUser::wearSticker(std::list<inventory::Item_v2>::iterator item, std::uint8_t slot)
{
    const auto skin = item->get<inventory::Skin>();
    if (!skin)
        return {};

    constexpr auto wearStep = 0.12f;
    const auto newWear = (skin->stickers[slot].wear += wearStep);

    if (const auto shouldRemove = (newWear >= 1.0f + wearStep)) {
        skin->stickers[slot] = {};
        return Response{ Response::StickerRemoved{ item, slot } };
    }

    return Response{ Response::StickerScraped{ item, slot } };
}

std::optional<Response> ToolUser::addNameTag(std::list<inventory::Item_v2>::iterator item, std::list<inventory::Item_v2>::const_iterator nameTagItem, std::string_view nameTag)
{
    const auto skin = item->get<inventory::Skin>();
    if (!skin)
        return {};

    skin->nameTag = nameTag;
    backend.removeItem(nameTagItem);
    backend.moveToFront(item);
    return Response{ Response::NameTagAdded{ item } };
}

std::optional<Response> ToolUser::removeNameTag(std::list<inventory::Item_v2>::iterator item)
{
    if (const auto skin = item->get<inventory::Skin>()) {
        skin->nameTag.clear();
        backend.moveToFront(item);
        return Response{ Response::NameTagRemoved{ item } };
    }
    return {};
}

std::optional<Response> ToolUser::openContainer(std::list<inventory::Item_v2>::const_iterator container, std::optional<std::list<inventory::Item_v2>::const_iterator> key)
{
    if (!container->gameItem().isCase())
        return {};

    if (key.has_value()) {
        if (const auto& keyItem = *key; keyItem->gameItem().isCaseKey())
            backend.removeItem(keyItem);
    }

    auto generatedItem = ItemGenerator::generateItemFromContainer(*container);
    backend.removeItem(container);
    const auto receivedItem = backend.addItem(std::move(generatedItem));
    return Response{ Response::ContainerOpened{ receivedItem } };
}

}
